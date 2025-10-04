import matplotlib.pyplot as plt

# -------------------------------
# Data
# -------------------------------
# Reorder Buffer (ROB) estimation
chain_lengths = [16, 32, 64, 128, 256, 512, 1024]
# Sapphire Rapids (Artemisia)
rob_sr = [780, 1370, 412, 776, 1692, 2976, 5908]
# Sandy Bridge (Sunbird)
rob_sb = [464, 788, 1812, 3368, 6164, 15316, 30352]

# Register File Pressure
live_regs = [8, 16, 32, 64, 128]
# Sapphire Rapids
rf_sr = [14707324, 32569470, 55785302, 127262696, 272470904]
# Sandy Bridge
rf_sb = [18936325, 33515033, 62030464, 137654580, 257911784]

# -------------------------------
# Plotting ROB
# -------------------------------
plt.figure(figsize=(10,5))
plt.subplot(1,2,1)
plt.plot(chain_lengths, rob_sr, marker='o', color='blue', label='Sapphire Rapids')
plt.plot(chain_lengths, rob_sb, marker='o', color='orange', label='Sandy Bridge')
plt.xlabel('Dependency Chain Length')
plt.ylabel('Cycles')
plt.title('ROB Estimation')
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend()
plt.xticks(chain_lengths)

# Highlight plateau region (rough ROB size)
plt.axvspan(256, 512, color='gray', alpha=0.2)
plt.text(260, max(max(rob_sr), max(rob_sb))*0.9, 'Plateau → ROB capacity', fontsize=8, rotation=0)

# -------------------------------
# Plotting Register File Pressure
# -------------------------------
plt.subplot(1,2,2)
plt.plot(live_regs, rf_sr, marker='o', color='blue', label='Sapphire Rapids')
plt.plot(live_regs, rf_sb, marker='o', color='orange', label='Sandy Bridge')
plt.xlabel('Number of Live Registers')
plt.ylabel('Cycles')
plt.title('Register File Pressure Test')
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend()
plt.xticks(live_regs)

# Highlight saturation region
plt.axvspan(32, 128, color='gray', alpha=0.2)
plt.text(34, max(max(rf_sr), max(rf_sb))*0.7, 'Register file saturation', fontsize=8, rotation=90)

plt.tight_layout()
plt.show()
