import sys
import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt(sys.argv[1],delimiter=',')
print data

plt.plot(data[:,1],data[:,0],'o')

plt.show()
