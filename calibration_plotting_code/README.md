# Methane Calibration Curve-Fitting Program

## Introduction

This folder contains the Python curve-fitting program used to calibrate the methane sensing platform developed for the individual project **Dual-Gas Sensing Platform for Peatland Greenhouse Gas Research**.

The program fits the modified Beer-Lambert methane sensor response model using processed calibration data from an Excel workbook. The SPAN value is fixed using the experimentally determined value from pure-methane exposure, while the calibration coefficients `a` and `n` are fitted from the processed gas-concentration calibration data.

The purpose of this software is to reproduce the calibration curve-fitting process used to obtain the methane sensor calibration coefficients reported in the final project report.

## Folder structure

```text
calibration_plotting_code/
|
├── curvefit_fixedSpan.py
├── requirements.txt
├── README.md
└── data/
    └── Gas_calibration_data_all_git.xlsx
```

## Contextual overview

The methane sensing platform measures the active and reference channel peak-to-peak voltages from the IR12GJ NDIR methane sensor. These values are processed to calculate normalised absorbance, which is then related to methane concentration using a modified Beer-Lambert model.

This curve-fitting program uses processed calibration data rather than raw Arduino and UGGA log files. The processed Excel worksheet contains methane concentration values and corresponding averaged normalised absorbance values.

Simplified workflow:

```text
Processed calibration Excel worksheet
        ↓
Read concentration and normalised absorbance columns
        ↓
Fit fixed-SPAN modified Beer-Lambert model
        ↓
Calculate fitted a and n coefficients
        ↓
Generate calibration plot and residual plot
        ↓
Save output figure as PDF
```

## Files

### `curvefit_fixedSpan.py`

Python script used to fit the methane calibration curve. It:

- loads processed calibration data from an Excel worksheet,
- checks that the required columns are present,
- fits the modified Beer-Lambert model with fixed SPAN,
- calculates fit-quality metrics,
- generates a side-by-side calibration and residual plot,
- saves the output figure as a PDF.

### `data/Gas_calibration_data_all_git.xlsx`

Processed calibration workbook used by the script.

The default worksheet is:

```text
Feb23_mod
```

The required input columns are:

```text
Concentration (%vol)
NA_avg
```

A different processed calibration worksheet can be selected by changing the `DEFAULT_SHEET` value in the Python script.

### `requirements.txt`

Lists the Python packages required to run the program.

## Installation instructions

This program requires Python and the packages listed in `requirements.txt`.

Python 3.10 or later is recommended.

From inside the `calibration_plotting_code` folder, install the required packages using:

```bash
pip install -r requirements.txt
```

The required packages are:

```text
numpy
pandas
matplotlib
scipy
openpyxl
```

`openpyxl` is required because the input calibration data is stored in an `.xlsx` Excel workbook.

## How to run the software

From the `calibration_plotting_code` folder, run:

```bash
python curvefit_fixedSpan.py
```

The script reads the default workbook:

```text
data/Gas_calibration_data_all_git.xlsx
```

and the default worksheet:

```text
Feb23_mod
```

After running, the script prints the fitted parameters and fit-quality metrics in the terminal.

The output figure is saved to:

```text
outputs/calibration_fit_and_residuals_side_by_side.pdf
```

## Optional command-line examples

Display the figure interactively after saving:

```bash
python curvefit_fixedSpan.py --show
```

Select a different worksheet:

```bash
python curvefit_fixedSpan.py --sheet Feb23_mod
```

Use a different fixed SPAN value:

```bash
python curvefit_fixedSpan.py --span 0.36154
```

Use a different Excel workbook:

```bash
python curvefit_fixedSpan.py --file path/to/workbook.xlsx
```

## Technical details

The fitted model is the fixed-SPAN modified Beer-Lambert response model:

```text
NA = SPAN × (1 - exp(-a × c^n))
```

where:

- `NA` is the normalised absorbance,
- `SPAN` is fixed at `0.36154` by default,
- `a` is a fitted linearisation coefficient,
- `n` is a fitted exponent coefficient,
- `c` is methane concentration in % v/v.

The script fits only `a` and `n`. The SPAN value is not fitted because it was determined separately from pure-methane exposure during the project.

The script calculates the following fit-quality metrics:

- coefficient of determination, `R²`,
- root-mean-square error, `RMSE`,
- mean absolute error, `MAE`.

The generated figure contains:

1. calibration data with the fitted model curve,
2. residuals plotted against methane concentration.

## Design assumptions

The program assumes that:

- the input Excel workbook contains processed calibration data,
- the selected worksheet contains the required columns `Concentration (%vol)` and `NA_avg`,
- methane concentration is given in % v/v,
- normalised absorbance has already been calculated during data preprocessing,
- the fixed SPAN value is valid for the sensor and hardware configuration used in the project,
- the processed calibration data is suitable for fitting the modified Beer-Lambert response model.

## Known issues and limitations

- The program uses processed calibration data rather than raw Arduino and UGGA log files.
- The default worksheet is `Feb23_mod`; other worksheets must be selected manually.
- The fixed SPAN value is hard-coded by default as `0.36154`.
- The fitted coefficients are specific to the IR12GJ sensor, PCB, calibration setup, and processed dataset used in this project.
- The generated calibration model should not be assumed valid for a different sensor, PCB, optical setup, or calibration procedure without recalibration.

## Future improvements

Possible improvements include:

- adding raw-data preprocessing from Arduino and UGGA log files,
- adding automatic worksheet detection,
- storing calibration constants in a separate configuration file,
- adding automated checks for concentration range and outliers,

## Academic context

This software was developed as part of the final-year individual project. It is included to make the calibration curve-fitting workflow reproducible and to allow inspection of the Python code and processed calibration data used to obtain the reported methane calibration coefficients.
