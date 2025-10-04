import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ------------------------------
# Load data
# ------------------------------
artemisia = pd.read_csv(
    "/Users/varun/Documents/NC_State_work/Class/ECE592-Microarch Sec/Homework/hw2/ece-592-microarch-sec-rev-eng-hw2/5.10/artemisia/ht_results.csv"
)
sunbird = pd.read_csv(
    "/Users/varun/Documents/NC_State_work/Class/ECE592-Microarch Sec/Homework/hw2/ece-592-microarch-sec-rev-eng-hw2/5.10/sunbird/ht_results.csv"
)

# Ensure numeric types (convert from strings if needed)
artemisia["Cycles"] = pd.to_numeric(artemisia["Cycles"], errors="coerce")
sunbird["Cycles"] = pd.to_numeric(sunbird["Cycles"], errors="coerce")

# Drop invalid entries
artemisia = artemisia.dropna(subset=["Cycles"])
sunbird = sunbird.dropna(subset=["Cycles"])

# ------------------------------
# Helper to summarize
# ------------------------------
def summarize(df):
    return df.groupby(["Mode", "Condition"])["Cycles"].agg(["mean", "std"]).reset_index()


a_sum = summarize(artemisia)
s_sum = summarize(sunbird)

# ------------------------------
# Plotting
# ------------------------------
fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=False)
systems = [("Sapphire Rapids", a_sum), ("Sandy Bridge", s_sum)]

for ax, (title, df) in zip(axes, systems):
    modes = df["Mode"].unique()
    x = np.arange(len(modes))
    width = 0.35

    base = df[df["Condition"] == "Baseline"]
    stress = df[df["Condition"] == "Stress"]

    # Bar plots with error bars
    ax.bar(
        x - width / 2,
        base["mean"],
        width,
        yerr=base["std"],
        capsize=4,
        label="Baseline",
        color="steelblue",
        alpha=0.8,
    )
    ax.bar(
        x + width / 2,
        stress["mean"],
        width,
        yerr=stress["std"],
        capsize=4,
        label="Stress (HT active)",
        color="crimson",
        alpha=0.8,
    )

    ax.set_xticks(x)
    ax.set_xticklabels(modes, fontsize=12)
    ax.set_title(title, fontsize=13, weight="bold")
    ax.set_ylabel("Cycles (log scale)", fontsize=12)
    ax.set_yscale("log")  # <-- key fix
    ax.grid(True, linestyle="--", alpha=0.5, which="both")
    ax.legend()

plt.suptitle(
    "Hyper-Threading Partitioning: ROB & BTB Interference",
    fontsize=15,
    weight="bold",
)
plt.tight_layout()
plt.show()
