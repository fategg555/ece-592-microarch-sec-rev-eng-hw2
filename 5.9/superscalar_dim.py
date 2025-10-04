import matplotlib.pyplot as plt

# -----------------------------
# Data
# -----------------------------
chain_lengths = [16, 32, 64, 128, 256, 512]

# Average cycles per op from dependent chains
sapphire_cycles = [19.00, 11.88, 5.81, 5.02, 2.88, 6.11]
sandy_cycles    = [9.88, 6.88, 6.58, 7.30, 7.68, 7.80]

# Max instructions per cycle (IPC)
max_ipc = {
    'Sapphire Rapids': 1.03,
    'Sandy Bridge': 0.79
}

# -----------------------------
# Plot: Pipeline Depth
# -----------------------------
plt.figure(figsize=(7,5))
plt.plot(chain_lengths, sapphire_cycles, marker='o', label='Sapphire Rapids', color='blue')
plt.plot(chain_lengths, sandy_cycles, marker='s', label='Sandy Bridge', color='orange')

# Highlight possible pipeline depth region (lowest latency per op)
plt.scatter(128, 5.02, s=100, facecolors='none', edgecolors='blue', linewidths=2, label='Sapphire depth region')
plt.scatter(64, 6.58, s=100, facecolors='none', edgecolors='orange', linewidths=2, label='Sandy depth region')

plt.title("Superscalar Pipeline Depth (Avg cycles per op)")
plt.xlabel("Dependent Chain Length")
plt.ylabel("Avg Cycles per Operation")
plt.xticks(chain_lengths)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.show()

# -----------------------------
# Plot: Max IPC Bar Chart
# -----------------------------
plt.figure(figsize=(5,4))
plt.bar(max_ipc.keys(), max_ipc.values(), color=['blue', 'orange'])
plt.title("Max Instructions per Cycle")
plt.ylabel("IPC")
plt.ylim(0, max(max_ipc.values()) + 0.5)

# Add value labels on top
for i, v in enumerate(max_ipc.values()):
    plt.text(i, v + 0.05, f"{v:.2f}", ha='center', fontweight='bold')

plt.tight_layout()
plt.show()
