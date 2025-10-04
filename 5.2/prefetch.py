import matplotlib.pyplot as plt
import numpy as np

# CPU names
cpus = ["Sapphire Rapids", "Sandy Bridge"]

# Streaming and randomized access times in cycles
streaming = [77151062, 163957513]       # Sapphire Rapids, Sandy Bridge
randomized = [1541766674, 1016343096]   # Sapphire Rapids, Sandy Bridge

# X positions for the grouped bar chart
x = np.arange(len(cpus))
width = 0.35

# Create figure and axes
fig, ax = plt.subplots(figsize=(8,5))

# Plot streaming and randomized bars
bars1 = ax.bar(x - width/2, streaming, width, label='Streaming Access', color='skyblue')
bars2 = ax.bar(x + width/2, randomized, width, label='Randomized Access', color='salmon')

# Highlight max points with circles
for i in range(len(cpus)):
    ax.plot(x[i] - width/2, streaming[i], 'o', markersize=10, markeredgecolor='blue', markerfacecolor='none', markeredgewidth=2)
    ax.plot(x[i] + width/2, randomized[i], 'o', markersize=10, markeredgecolor='red', markerfacecolor='none', markeredgewidth=2)

# Set labels and title
ax.set_ylabel('Access Time (Cycles)')
ax.set_title('Memory Access Patterns: Streaming vs Randomized')
ax.set_xticks(x)
ax.set_xticklabels(cpus)
ax.legend()

# Optionally use logarithmic scale for better visibility
ax.set_yscale('log')

# Annotate cycle counts above bars
for i in range(len(cpus)):
    ax.text(x[i] - width/2, streaming[i]*1.1, f"{streaming[i]:,.0f}", ha='center', fontsize=9)
    ax.text(x[i] + width/2, randomized[i]*1.1, f"{randomized[i]:,.0f}", ha='center', fontsize=9)

plt.tight_layout()
plt.show()
