

# Homework 2
This is Homework 2, reverse engineering components of a Sandy Bridge and Sapphire Rapids systems. Report included in this repo.

---

## Table of Contents

-  [Installation](#installation-instructions)

-  [Usage](#usage)

--- 

## Installation Instructions
  

### Prerequisites

- Python 3.12.9
- gcc 13.3.0 (with g++)

### Steps

1.  **Clone the repository:**

```bash
git clone https://github.com/fategg555/ece-592-microarch-sec-rev-eng-hw2.git
```
3. **Install Python Virtual Environment (needed for 5.4 and image generation)**
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

# Usage
## Running Tests
### Prerequisities
- Python venv is activated
- `cd` into the test folder (e.g, 5.4)
### Steps
1. For tests with a `compile.sh`, run `./compile.sh` and then run the resulting executable
2. For tests with a `run.sh`, run `./run.sh` in the folder


