#pragma once
#include <queue>
#include <random>
#include <string>
#include <map>
#include "event_based/weight.h"
#include "event_based/neuron.h"
#include "event_based/neuron_params.h"
#include "event_based/rectsynapse.h"
#include "event_based/expsynapse.h"
#include "event_based/population.h"
#include "event_based/synapse_range.h"
#include "event_based/event.h"
#include "event_based/pre_syn_event.h"
#include "event_based/post_syn_event.h"
#include "event_based/psp_event.h"
#include "event_based/silence_event.h"

namespace xnet {
	template<typename SYN, typename WT>
	class SimulationQueue
	{
	public:
		SimulationQueue ();
		void run(size_t num);
		// population stuff
		Population create_population_start(std::size_t size);
		void create_population_add_neuron(Neuron_params const& p);
		Population create_population_fixed(std::size_t s, Neuron_params const& params);
		Population create_population_uniform(
			std::size_t s,
			UniformRange_t th,
			UniformRange_t tm,
			UniformRange_t tr,
			UniformRange_t ti,
			UniformRange_t td
		);
		Population create_population_normal(
			std::size_t s,
			NormalRange_t th,
			NormalRange_t tm,
			NormalRange_t tr,
			NormalRange_t ti,
			NormalRange_t td
		);
		// connection stuff
		void connect_all_to_all_identical(Population const& p1, Population const& p2, WT const& w, Timeconst_t ltp);
		void connect_one_to_one_identical(Population const& p1, Population const& p2, WT const& w, Timeconst_t ltp);
		void connect_all_to_all_normal(
			Population const& p1,
			Population const& p2,
			NormalRange_t wmin,
			NormalRange_t wmax,
			NormalRange_t winit,
			NormalRange_t ap,
			NormalRange_t am,
			NormalRange_t ltp
		);
		void connect_all_to_all_wta(Population const& p);

		void add_event(event * e);
		void run_until_empty();
		void run_one_event();
		std::vector<SynapseRange> get_synapse_ranges(Id_t const& neuron) const;
		std::vector<Id_t> get_pre_synapse_ranges(Id_t const& neuron) const;
		SYN* get_synapse_pointer(Id_t const& synapse);
		SYN const* get_synapse_pointer(Id_t const& synapse) const;
		Neuron* get_neuron_pointer(Id_t const& nrn);
		Time_t get_time() const;
		void add_spike(Time_t t, Id_t nrn);
		std::vector<Spike_t>& get_spikes();
		std::vector<Spike_t> get_new_spikes();
		void print_spikes(std::string filename, Realtime_t scale=1.0);
		void print_pre_weights(Population const& pop, std::string filename) const;
		void print_pre_weights(Id_t nrn, std::string filename) const;
	protected:
		void processEvent(event* ev);
		void add_synapse(Id_t, Id_t, WT, Timeconst_t);
		void flush_psp_cache();
		void add_psp_cache(Id_t neuron, Current_t current);

		std::default_random_engine generator;
		unsigned int number_of_neurons;
		std::vector<Neuron> neurons;
		std::vector<SYN> synapses;
		std::map<Id_t,Current_t> psp_cache;
		Time_t cache_valid_time;
		bool cache_dirty;
		std::vector<std::vector<SynapseRange>> pre_syn_lookup;
		std::vector<std::vector<Id_t>> post_syn_lookup;
		Time_t time;
		std::priority_queue<event*, std::vector<event*, std::allocator<event*> >,
			eventComparator> eventQueue;
		std::vector<Spike_t> spike_list;
		std::size_t last_spike_fetched;
	};

	typedef SimulationQueue<RectSynapse,RectWeight> Simulation;
}
