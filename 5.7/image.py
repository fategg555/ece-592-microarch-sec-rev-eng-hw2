import matplotlib.pyplot as plt
import matplotlib.image as mpimg

# Load your images
# Replace 'image1.jpg' and 'image2.png' with your actual image file paths
image1 = mpimg.imread('sunbird/sunbird.png')
image2 = mpimg.imread('artemisia/artemisia.png')

# Create a figure and two subplots side-by-side
fig, axes = plt.subplots(1, 2, figsize=(10, 5)) # Adjust figsize as desired

# Display Image 1 in the first subplot
axes[0].imshow(image1)
axes[0].set_title('Sandy Bridge')
axes[0].axis('off') # Turn off axis ticks and labels

# Display Image 2 in the second subplot
axes[1].imshow(image2)
axes[1].set_title('Sapphire Rapids')
axes[1].axis('off') # Turn off axis ticks and labels

# Adjust layout to prevent overlap
plt.tight_layout()

# Show the plot
plt.show()
