import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

# -------------------------------
# Data
# -------------------------------
# ROB estimation (cycles)
chain_lengths = [16, 32, 64, 128, 256, 512, 1024]
rob_sr = [730, 1384, 612, 794, 1722, 3162, 5886]   # Sapphire Rapids
rob_sb = [725, 1225, 2794, 5987, 9625, 19232, 47988] # Sandy Bridge

# Register File Pressure (cycles)
live_regs = [8, 16, 32, 64]
rf_sr = [1500178, 2011770, 6907166, 7183064]    # Sapphire Rapids
rf_sb = [1898425, 3125637, 12697010, 12378670]  # Sandy Bridge

# -------------------------------
# Plotting
# -------------------------------
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

# --- ROB Estimation ---
ax1.plot(chain_lengths, rob_sr, marker='o', color='blue', label='Sapphire Rapids')
ax1.plot(chain_lengths, rob_sb, marker='o', color='orange', label='Sandy Bridge')
ax1.set_xlabel('Dependency Chain Length')
ax1.set_ylabel('Cycles')
ax1.set_title('Reorder Buffer (ROB) Estimation')
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend()

# Use integer locator for x-axis to prevent overlap
ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
ax1.set_xticks(chain_lengths)
ax1.tick_params(axis='x', rotation=30)  # rotate x-axis labels for spacing

# Highlight plateau region (ROB capacity)
ax1.axvspan(256, 512, color='gray', alpha=0.2)
ax1.text(384, max(max(rob_sr), max(rob_sb))*0.85, 'Plateau → ROB capacity', fontsize=9, ha='center')

# --- Register File Pressure ---
ax2.plot(live_regs, rf_sr, marker='o', color='blue', label='Sapphire Rapids')
ax2.plot(live_regs, rf_sb, marker='o', color='orange', label='Sandy Bridge')
ax2.set_xlabel('Number of Live Registers')
ax2.set_ylabel('Cycles')
ax2.set_title('Register File Pressure Test')
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend()

# Use integer locator for x-axis to prevent overlap
ax2.xaxis.set_major_locator(MaxNLocator(integer=True))
ax2.set_xticks(live_regs)
ax2.tick_params(axis='x', rotation=0)

# Highlight register file saturation
ax2.axvspan(16, 32, color='gray', alpha=0.2)  # Sapphire Rapids saturation
ax2.axvspan(32, 64, color='gray', alpha=0.1)  # Sandy Bridge saturation
ax2.text(24, max(max(rf_sr), max(rf_sb))*0.7, 'Register file saturation', fontsize=9, rotation=90, va='center')

plt.tight_layout()
plt.show()
