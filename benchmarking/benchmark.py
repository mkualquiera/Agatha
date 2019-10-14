import matplotlib.pyplot as plt
import numpy as np

f = open("../benchmark", "r")

data = {}

for l in f:
    cols = l.split(" ");
    if not cols[0] in data:
        data[cols[0]] = 0
    data[cols[0]] += float(cols[5]) / 100

names = []
averages = []
count = 0;

for x,y in data.items():
    print(x, y)
    names.append(x)
    averages.append(y)
    plt.bar(count, [y], 0.4, label=x)
    count+=1

plt.rcParams.update({'font.size': 18})
ax = plt.gca()
plt.yticks(fontsize=18)
ax.set_xticks(np.arange(len(names)))
ax.set_xticklabels(names, fontsize=18)
ax.set_title("Time taken to find best split")
ax.set_ylabel("Time (sec)", fontsize=18)
ax.set_xlabel("Dataset",fontsize=18)


plt.show()
