import pandas as pd
import matplotlib.pyplot as plt
import sys

# Replace 'data.csv' with your file name
# Replace 'column_name' with the name of the column you want to plot
file_path = sys.argv[1]
column_name = 'access_times'

# Load the data
data = pd.read_csv(file_path)
max_access = int(data[column_name].max())
min_access = int(data[column_name].min())
print(max_access)
# Plot the histogram
plt.hist(data[column_name].dropna(), bins=int((max_access - min_access)/10), edgecolor='black')
plt.title(f'Frequency of Access Times for Page-Aligned Linked List')
plt.xlabel(column_name)
plt.ylabel('Frequency')
plt.show()

