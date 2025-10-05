import matplotlib.pyplot as plt

# -------------------------------
# Data
# -------------------------------
# Reorder Buffer (ROB) estimation
chain_lengths = [16, 32, 64, 128, 256, 512, 1024]
rob_sr = [780, 1370, 412, 776, 1692, 2976, 5908]   # Sapphire Rapids
rob_sb = [464, 788, 1812, 3368, 6164, 15316, 30352] # Sandy Bridge

# Register File Pressure
live_regs = [8, 16, 32, 64, 128]
rf_sr = [14707324, 32569470, 55785302, 127262696, 272470904]  # Sapphire Rapids
rf_sb = [18936325, 33515033, 62030464, 137654580, 257911784]  # Sandy Bridge

# -------------------------------
# Combined Figure
# -------------------------------
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10,8))  # wider figure

# --- ROB Estimation ---
ax1.plot(chain_lengths, rob_sr, marker='o', color='blue', label='Sapphire Rapids')
ax1.plot(chain_lengths, rob_sb, marker='o', color='orange', label='Sandy Bridge')
ax1.set_xlabel('Dependency Chain Length')
ax1.set_ylabel('Cycles')
ax1.set_title('Reorder Buffer (ROB) Estimation')
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend()
ax1.set_xticks(chain_lengths)
ax1.tick_params(axis='x', rotation=30, labelsize=9)  # rotate & shrink font

# Highlight plateau region (ROB capacity)
ax1.axvspan(256, 512, color='gray', alpha=0.2)
ax1.text(300, max(max(rob_sr), max(rob_sb))*0.85, 
         'Plateau → ROB capacity', fontsize=9, ha='center')

# --- Register File Pressure ---
ax2.plot(live_regs, rf_sr, marker='o', color='blue', label='Sapphire Rapids')
ax2.plot(live_regs, rf_sb, marker='o', color='orange', label='Sandy Bridge')
ax2.set_xlabel('Number of Live Registers')
ax2.set_ylabel('Cycles')
ax2.set_title('Register File Pressure Test')
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend()
ax2.set_xticks(live_regs)
ax2.tick_params(axis='x', labelsize=9)

# Highlight saturation region
ax2.axvspan(32, 128, color='gray', alpha=0.2)
ax2.text(80, max(max(rf_sr), max(rf_sb))*0.7, 
         'Register file saturation', fontsize=9, rotation=90, va='center')

plt.tight_layout()
plt.show()
