import matplotlib.pyplot as plt

# Results from both systems
systems = ["Sunbird", "Artemisia"]

# Throughput metrics
cpi_values = [0.6957, 0.5457]
ipc_values = [1.4373, 1.8324]

# Latency metrics
latency_values = [0.95, 1.26]

# Print summary
print("=== AVX2 Benchmark Comparison ===")
for i in range(2):
    print(f"{systems[i]}:")
    print(f"  CPI: {cpi_values[i]:.4f}")
    print(f"  IPC: {ipc_values[i]:.4f}")
    print(f"  Latency per op: {latency_values[i]:.2f} cycles\n")

# Plotting
fig, ax = plt.subplots(1, 2, figsize=(12, 5))

# Throughput bar chart
ax[0].bar(systems, cpi_values, label="CPI", color="steelblue")
ax[0].bar(systems, ipc_values, label="IPC", color="orange", bottom=cpi_values)
ax[0].set_title("AVX2 Throughput Comparison")
ax[0].set_ylabel("Cycles / Instructions")
ax[0].legend()
ax[0].grid(True, linestyle="--", alpha=0.5)

# Latency bar chart
ax[1].bar(systems, latency_values, color="crimson")
ax[1].set_title("AVX2 Latency Comparison")
ax[1].set_ylabel("Cycles per Operation")
ax[1].grid(True, linestyle="--", alpha=0.5)

plt.tight_layout()
plt.show()