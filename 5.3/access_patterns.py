import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# -----------------------------------
# Load CSVs for Artemisia and Sunbird
# -----------------------------------
artemisia_csv = "/Users/varun/Documents/NC_State_work/Class/ECE592-Microarch Sec/Homework/hw2/ece-592-microarch-sec-rev-eng-hw2/5.3/artemisia/access_patterns.csv"
sunbird_csv   = "/Users/varun/Documents/NC_State_work/Class/ECE592-Microarch Sec/Homework/hw2/ece-592-microarch-sec-rev-eng-hw2/5.3/sunbird/access_patterns.csv"

art = pd.read_csv(artemisia_csv)
sun = pd.read_csv(sunbird_csv)

# Sort to ensure proper order
art = art.sort_values(["size", "stride"])
sun = sun.sort_values(["size", "stride"])

# -----------------------------------
# Compute summary statistics
# -----------------------------------
# Average latency per stride (aggregate across all sizes)
stride_means_art = art.groupby("stride")["cycles"].mean()
stride_means_sun = sun.groupby("stride")["cycles"].mean()

# Average latency per working-set size
size_means_art = art.groupby("size")["cycles"].mean()
size_means_sun = sun.groupby("size")["cycles"].mean()

# System-wide averages
art_mean = art["cycles"].mean()
sun_mean = sun["cycles"].mean()

# -----------------------------------
# Print summary to console
# -----------------------------------
print("=== Cache Access Characterization Summary (Q5.3.5) ===")
print(f"Sapphire Rapids average latency: {art_mean:.2f} cycles/access")
print(f"Sandy Bridge average latency:   {sun_mean:.2f} cycles/access\n")

print("Stride-wise latency (cycles/access):")
for stride in stride_means_art.index:
    print(f"  Stride {stride:>4} B:  Artemisia={stride_means_art[stride]:.2f}, Sunbird={stride_means_sun[stride]:.2f}")

print("\nSize-wise latency (cycles/access):")
for size in size_means_art.index:
    print(f"  Size {size//1024:>5} KB:  Artemisia={size_means_art[size]:.2f}, Sunbird={size_means_sun[size]:.2f}")

# -----------------------------------
# Plot setup
# -----------------------------------
plt.rcParams.update({
    "font.size": 12,
    "axes.titlesize": 14,
    "axes.labelsize": 12,
    "legend.fontsize": 10,
    "xtick.labelsize": 11,
    "ytick.labelsize": 11
})

fig, ax = plt.subplots(1, 2, figsize=(12, 5))

# -----------------------------------
# Left plot: Latency vs Stride
# -----------------------------------
x = np.arange(len(stride_means_art.index))
width = 0.35

ax[0].bar(x - width/2, stride_means_sun.values, width, label="Sandy Bridge", color="royalblue")
ax[0].bar(x + width/2, stride_means_art.values, width, label="Sapphire Rapids", color="darkorange")
ax[0].set_xticks(x)
ax[0].set_xticklabels(stride_means_art.index)
ax[0].set_xlabel("Stride (bytes)")
ax[0].set_ylabel("Avg Cycles per Access")
ax[0].set_title("Access Latency vs Stride")
ax[0].grid(True, linestyle="--", alpha=0.6)
ax[0].legend()

# -----------------------------------
# Right plot: Latency vs Working-set Size
# -----------------------------------
x2 = np.arange(len(size_means_art.index))
ax[1].bar(x2 - width/2, size_means_sun.values, width, label="Sandy Bridge", color="royalblue")
ax[1].bar(x2 + width/2, size_means_art.values, width, label="Sapphire Rapids", color="darkorange")
ax[1].set_xticks(x2)
ax[1].set_xticklabels([f"{s//1024}K" for s in size_means_art.index], rotation=30)
ax[1].set_xlabel("Working-set Size (KB)")
ax[1].set_ylabel("Avg Cycles per Access")
ax[1].set_title("Access Latency vs Working-set Size")
ax[1].grid(True, linestyle="--", alpha=0.6)
ax[1].legend()

# -----------------------------------
# Final layout
# -----------------------------------
plt.suptitle("Cache Access Characterization (Q5.3.5): Sapphire Rapids vs Sandy Bridge", fontsize=15, fontweight="bold")
plt.tight_layout(rect=[0, 0, 1, 0.96])
plt.savefig("access_patterns_comparison_clean.png", dpi=300)
plt.show()
