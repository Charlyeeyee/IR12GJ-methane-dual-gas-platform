# Calibration and Plotting Code

This folder contains the Python script used to fit the methane sensor calibration curve and generate the calibration figure used in the final report.

The script processes prepared calibration data from an Excel workbook. The required input columns are methane concentration in `% v/v` and averaged normalised absorbance (`NA_avg`). The script fits the modified Beer–Lambert response model using a fixed experimentally determined SPAN value, then estimates the linearisation coefficients `a` and `n`.

## Main functions

- Load processed calibration data from an Excel worksheet.
- Extract methane concentration and averaged normalised absorbance data.
- Fit the methane sensor response model using a fixed SPAN value.
- Estimate the linearisation coefficients `a` and `n` using non-linear curve fitting.
- Calculate goodness-of-fit metrics including R², RMSE, and MAE.
- Calculate residuals between measured and fitted normalised absorbance.
- Generate a side-by-side figure showing:
  - the calibration data and fitted curve;
  - the residuals against methane concentration.
- Save the final figure as a PDF file for use in the report.
- Print the fitted parameters and fit-quality metrics to the console.

## Input data

The script expects an Excel file containing the processed calibration data. In the current version, the main user settings are:

- Excel file: `Gas_callibration_data_all.xlsx`
- Sheet name: `Feb23_mod`
- Header row: `11`
- Fixed SPAN value: `0.36154`

The Excel sheet must contain the following columns:

- `Concentration (%vol)`
- `NA_avg`

## Model used

The fitted model is:

`NA = SPAN_FIXED × (1 - exp(-a × c^n))`

where:

- `NA` is the normalised absorbance;
- `SPAN_FIXED` is the fixed span value determined experimentally;
- `c` is methane concentration in `% v/v`;
- `a` and `n` are fitted linearisation coefficients.

## Output

The script outputs:

- fitted values of `a` and `n`;
- standard errors for the fitted parameters;
- R², RMSE, and MAE;
- a PDF figure named `calibration_fit_and_residuals_side_by_side.pdf`.

## Notes

This script was developed for the final calibration workflow used in the project. The Excel file name, sheet name, header row, column names, fixed SPAN value, and output figure name may need to be changed before reuse with another dataset. Some preprocessing steps, such as calculating `NA_avg`, may be carried out separately in Excel before running this script.
