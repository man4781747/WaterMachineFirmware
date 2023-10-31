import requests
import datetime 
import re
import matplotlib.pyplot as plt
import numpy as np
import math

I_maxValue = 20000

L_AList = []

for i in range(I_maxValue):
  L_AList.append(-math.log10( (i+1)/I_maxValue ))

L_dAList = []
for i in range(I_maxValue-1):
  L_dAList.append(-L_AList[i+1]+L_AList[i])

Ar_AList = np.array(L_AList)
I_maxA = 0.5
I_maxPPM = 1
Ar_Range = np.where(Ar_AList<I_maxA)

plt.scatter(
  range(I_maxValue), 
  L_AList
)

# plt.scatter(
#   range(I_maxValue-1), 
#   L_dAList
# )

plt.show()