#include <fstream>
#include <tuple>
#include "event_based/logger.h"
#include "event_based/simulation_queue.h"

xnet::Simulation theSimulation;

namespace xnet {

	void Simulation::run_one_event()
	{
		LOGGER("eventQueue size: " << eventQueue.size());
		event * nextEvent = eventQueue.top();
		eventQueue.pop();
		time = nextEvent->time;
		//nextEvent->processEvent();
		processEvent(nextEvent);
		delete nextEvent;
		LOGGER("eventQueue size: " << eventQueue.size());
	}

	void Simulation::run(size_t num) {
		for (unsigned int i=0; i<num; ++i)
		{
			if (!eventQueue.empty())
				run_one_event();
		}
	}

	void Simulation::processEvent(event* ev) {
		auto type = ev->get_type();
		Id_t linked_object = ev->get_linked_object_id();
		if (type == EventType::PRE)
		{
			// add this spike to the global spike list
			add_spike(time,linked_object);
			// check if there are post-synaptic neurons to this neuron
			SynapseRange synrange = get_synapse_range(linked_object);
			if (synrange.non_empty())
			{
				LOGGER("@" << time << " Processing pre_syn_event for neuron " << linked_object << " and synrange from "<< synrange.begin() << " to " << synrange.end());
				for (std::size_t i=synrange.begin(); i<synrange.end(); ++i)
				{
					Synapse* syn = get_synapse_pointer(i);
					LOGGER("Current: " << syn->get_current());
					add_event(new psp_event(time,syn->get_post_neuron(),syn->get_current()));
				}
			}
			else
				LOGGER("@" << time << " Processing pre_syn_event for neuron. Empty, not doing anything.");
		}
		else if (type == EventType::PSP)
		{
			psp_event* psp_evt = static_cast<psp_event*>(ev);

			LOGGER("@" << time << " Processing psp event for post-synaptic neuron " << linked_object);
			Neuron* neuron = get_neuron_pointer(linked_object);
			bool fired = neuron->add_current_evolve(time,psp_evt->get_current());
			if (fired)
			{
				LOGGER("Neuron " << linked_object << " fired");
				add_event(new pre_syn_event(time,linked_object));
			}
		}
		//	case EventType::PST:
		//	break;
		//	case EventType::SIL:
		//	break;
		//}
	}

	Population Simulation::create_population_start()
	{
		Population pop;
		pop.set_start(neurons.size());
		return pop;
	}

	void Simulation::create_population_add_neuron(Neuron_params const& p)
	{
		neurons.push_back(Neuron(p));
		pre_syn_lookup.push_back({});
	}

	Population Simulation::create_population_fixed(std::size_t s, Neuron_params const& params)
	{
		/* Create a population and a number of s neurons.
			The population represents the range of the
			then created neurons on the global neuron stack.
			Additionally, there is a SynapseRange object
			createrd for every neuron.
		*/
		Population pop = create_population_start();
		for (unsigned int i=0;i<s;++i)
		{
			create_population_add_neuron(params);
		}
		pop.set_end(neurons.size()-1);
		return pop;
	}

	Population Simulation::create_population_uniform(std::size_t s, UniformRange_t th, UniformRange_t tm, UniformRange_t tr)
	{
		/* Create a population and a number of s neurons.
			The population represents the range of the
			then created neurons on the global neuron stack.
			Additionally, there is a SynapseRange object
			createrd for every neuron.
		*/
		std::uniform_real_distribution<Membrane_t> V_th(th.low(),th.high());
		std::uniform_real_distribution<Timeconst_t> tau_mem(tm.low(),tm.high());
		std::uniform_real_distribution<Timeconst_t> tau_ref(tr.low(),tr.high());
		Population pop = create_population_start();
		for (unsigned int i=0;i<s;++i)
		{
			create_population_add_neuron({V_th(generator),tau_mem(generator),tau_ref(generator)});
		}
		pop.set_end(neurons.size()-1);
		return pop;
	}

	Population Simulation::create_population_normal(std::size_t s, NormalRange_t th, NormalRange_t tm, NormalRange_t tr)
	{
		/* Create a population and a number of s neurons.
			The population represents the range of the
			then created neurons on the global neuron stack.
			Additionally, there is a SynapseRange object
			createrd for every neuron.
		*/
		std::normal_distribution<Membrane_t> V_th(th.mean(),th.std());
		std::normal_distribution<Timeconst_t> tau_mem(tm.mean(),tm.std());
		std::normal_distribution<Timeconst_t> tau_ref(tr.mean(),tr.std());
		Population pop = create_population_start();
		for (unsigned int i=0;i<s;++i)
		{
			create_population_add_neuron({V_th(generator),tau_mem(generator),tau_ref(generator)});
		}
		pop.set_end(neurons.size()-1);
		return pop;
	}

	void Simulation::connect_all_to_all_identical(Population& p1, Population& p2, Weight const& w)
	{
		// iterate over source neurons
		for (unsigned int i=0; i<p1.size(); ++i)
		{
			auto p1_index = p1.get(i);
			// store synapse range in pre_syn_lookup
			pre_syn_lookup[p1_index].set_start(synapses.size());
			// iterate over target neurons
			for (unsigned int j=0; j<p2.size(); ++j)
			{
				synapses.push_back(Synapse(p1_index,p2.get(j),w));
			}
			pre_syn_lookup[p1_index].set_end(synapses.size()-1);
		}
	}

	void Simulation::connect_all_to_all_normal(
		Population& p1,
		Population& p2,
		NormalRange_t wmin,
		NormalRange_t wmax,
		NormalRange_t winit,
		NormalRange_t ap,
		NormalRange_t am
	)
	{
		// distributions to sample from
		std::normal_distribution<Current_t> wmin_dist(wmin.mean(),wmin.std());
		std::normal_distribution<Current_t> wmax_dist(wmax.mean(),wmax.std());
		std::normal_distribution<Current_t> winit_dist(winit.mean(),winit.std());
		std::normal_distribution<Current_t> ap_dist(ap.mean(),ap.std());
		std::normal_distribution<Current_t> am_dist(am.mean(),am.std());

		// iterate over source neurons
		for (unsigned int i=0; i<p1.size(); ++i)
		{
			auto p1_index = p1.get(i);
			// store synapse range in pre_syn_lookup
			pre_syn_lookup[p1_index].set_start(synapses.size());
			// iterate over target neurons
			for (unsigned int j=0; j<p2.size(); ++j)
			{
				synapses.push_back(Synapse(p1_index,p2.get(j),{winit_dist(generator),wmin_dist(generator),wmax_dist(generator),ap_dist(generator),am_dist(generator)}));
			}
			pre_syn_lookup[p1_index].set_end(synapses.size()-1);
		}
	}

	void Simulation::add_event(event * e)
	{
		eventQueue.push(e);
		LOGGER("Adding event for time " << e->time << " object " << e->get_linked_object_id());
	}

	void Simulation::run_until_empty()
	{
		while (!eventQueue.empty()) {
			run_one_event();
		}
	}

	SynapseRange Simulation::get_synapse_range(Id_t const& neuron) const
	{
		return pre_syn_lookup[neuron];
	}

	Synapse* Simulation::get_synapse_pointer(Id_t const& synapse)
	{
		return &(synapses[synapse]);
	}

	Neuron* Simulation::get_neuron_pointer(Id_t const& nrn)
	{
		return &(neurons[nrn]);
	}

	Time_t Simulation::get_time() const
	{
		return time;
	}

	std::vector<Spike_t>& Simulation::get_spikes()
	{
		return spike_list;
	}

	void Simulation::add_spike(Time_t t, Id_t nrn)
	{
		spike_list.push_back(std::make_tuple(t,nrn));
	}

	void Simulation::print_spikes(std::string filename)
	{
		std::ofstream file(filename,std::ios::out);
		for (auto pair : spike_list)
		{
			file << std::get<1>(pair) << "," << std::get<0>(pair) << "\n";
		}
		file.close();
	}
}
