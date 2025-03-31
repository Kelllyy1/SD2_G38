# AWS IoT SiteWise Asset Creation Scripts

These Python scripts help automate the creation of `Cell` and `Module` assets in AWS IoT SiteWise for a scalable battery monitoring system.

---

## Understanding Models vs Assets

- **Asset Models** (`Cell`, `Module`): Define the structure and properties of your data.  
  You **only need to create these once** using `cell.py` and `module.py`.

- **Assets**: Instances of those models (e.g., `Cell-C001`, `BatteryModule001`).  
  You can create and associate **as many as you need** for scaling your system.

---

## Prerequisites

- Python 3.7 or higher
- `boto3` installed:
  ```
  pip install boto3
  ```
- AWS CLI configured:
  ```
  aws configure
  ```

Make sure your IAM user has SiteWise permissions (create/list/describe/associate assets).

---

## Usage

### Step 1: Create Models (one-time setup)

Run each of these once to define your models in SiteWise:

```bash
python cell.py
python module.py
```

### Step 2: Create `Cell` Assets

Create any number of new `Cell` assets. This command will continue from the last number automatically:

```bash
python create_cells.py 5
```

This creates 5 new cells and saves their IDs to `cell_ids.txt`.

### Step 3: Create or Update a `Module` Asset

Create a new `Module` asset if one doesn't exist, or add new cells to an existing one:

```bash
python create_module.py
```

---

## Files

| File               | Purpose                                           |
|--------------------|---------------------------------------------------|
| `cell.py`          | Creates the `Cell` asset model                    |
| `module.py`        | Creates the `Module` asset model                  |
| `create_cells.py`  | Creates new `Cell` assets                         |
| `create_module.py` | Creates or updates a `Module` asset and links cells |
| `cell_ids.txt`     | Auto-generated list of new Cell asset IDs         |
