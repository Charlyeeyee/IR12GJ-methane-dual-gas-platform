"""
Fixed-SPAN methane sensor calibration curve fitting script.

This script loads processed calibration data from an Excel worksheet,
fits the modified Beer-Lambert response model with a fixed SPAN value,
calculates fit-quality metrics, and generates a side-by-side calibration
and residual plot.

Expected input columns:
    - Concentration (%vol)
    - NA_avg

Main output:
    - calibration_fit_and_residuals_side_by_side.pdf
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit


# =============================================================================
# USER SETTINGS
# =============================================================================

# Excel workbook containing the processed calibration data
FILE_NAME = "Gas_callibration_data_all.xlsx"

# Worksheet containing the calibration dataset to be fitted
SHEET = "Feb23_mod"

# Row number used by pandas as the column-header row
# Note: pandas uses zero-based indexing, so this corresponds to Excel row 12
HEADER_ROW = 11

# Fixed SPAN value determined experimentally from pure-methane exposure
SPAN_FIXED = 0.36154

# Optional figure title; set to None to omit a title
FIGURE_TITLE = None

# Output filename for the combined calibration and residual figure
COMBINED_FIGURE_FILE = "calibration_fit_and_residuals_side_by_side.pdf"

# Set True to generate the residual subplot alongside the calibration plot
PLOT_RESIDUALS = True


# =============================================================================
# PLOT STYLE
# =============================================================================

# Configure global Matplotlib settings for report-ready figure formatting
plt.rcParams.update({
    "font.family": "sans-serif",
    "font.size": 18,
    "axes.labelsize": 20,
    "axes.titlesize": 20,
    "xtick.labelsize": 18,
    "ytick.labelsize": 18,
    "legend.fontsize": 18
})


# =============================================================================
# MODEL DEFINITION
# =============================================================================

def model_fixed_span(c, a, n):
    """
    Modified Beer-Lambert response model with fixed SPAN.

    Parameters
    ----------
    c : array-like
        Methane concentration in % v/v.
    a : float
        Linearisation coefficient fitted from calibration data.
    n : float
        Exponent coefficient fitted from calibration data.

    Returns
    -------
    array-like
        Predicted normalised absorbance.
    """
    return SPAN_FIXED * (1 - np.exp(-a * c**n))


# Initial parameter guesses for non-linear curve fitting:
# a and n are started close to expected methane sensor values.
p0 = [0.3, 0.7]

# Lower and upper bounds for fitted parameters [a, n].
# These prevent the optimiser from selecting non-physical negative values.
bounds = ([0.0, 0.0], [5.0, 2.0])


# =============================================================================
# LOAD AND PREPARE DATA
# =============================================================================

# Load the selected worksheet from the Excel workbook
df = pd.read_excel(FILE_NAME, sheet_name=SHEET, header=HEADER_ROW)

# Remove leading/trailing spaces from column names to avoid matching errors
df.columns = df.columns.str.strip()

# Define the required input columns
required = {"Concentration (%vol)", "NA_avg"}

# Check whether any required columns are missing from the worksheet
missing = required - set(df.columns)
if missing:
    raise ValueError(f"Missing columns in sheet '{SHEET}': {missing}")

# Keep only the columns needed for curve fitting
df = df[["Concentration (%vol)", "NA_avg"]].copy()

# Replace infinite values with NaN, then remove incomplete rows
df = df.replace([np.inf, -np.inf], np.nan).dropna()

# Convert concentration and normalised absorbance columns to NumPy arrays
c_data = df["Concentration (%vol)"].astype(float).to_numpy()
y_data = df["NA_avg"].astype(float).to_numpy()

# Require at least three valid points to fit a two-parameter model sensibly
if len(c_data) < 3:
    raise ValueError("Not enough valid data points for curve fitting.")

# Sort data by concentration so the plot and fitted curve are ordered clearly
sort_idx = np.argsort(c_data)
c_data = c_data[sort_idx]
y_data = y_data[sort_idx]


# =============================================================================
# CURVE FITTING
# =============================================================================

# Fit the fixed-SPAN model to the calibration data.
# popt contains the best-fit parameters [a, n].
# pcov is the covariance matrix estimated by scipy.
popt, pcov = curve_fit(
    model_fixed_span,
    c_data,
    y_data,
    p0=p0,
    bounds=bounds,
    maxfev=50000
)

# Extract fitted coefficients
a_fit, n_fit = popt

# Estimate standard errors from the diagonal of the covariance matrix
param_se = np.sqrt(np.diag(pcov))
a_se, n_se = param_se


# =============================================================================
# GOODNESS-OF-FIT CALCULATIONS
# =============================================================================

# Calculate fitted model values at the measured concentration points
y_pred = model_fixed_span(c_data, a_fit, n_fit)

# Residuals are measured normalised absorbance minus fitted absorbance
residuals = y_data - y_pred

# Residual sum of squares
ss_res = np.sum(residuals**2)

# Total sum of squares relative to the mean measured absorbance
ss_tot = np.sum((y_data - np.mean(y_data))**2)

# Coefficient of determination, R²
r2 = 1 - (ss_res / ss_tot) if ss_tot != 0 else np.nan

# Root mean square error of normalised absorbance
rmse = np.sqrt(np.mean(residuals**2))

# Mean absolute error of normalised absorbance
mae = np.mean(np.abs(residuals))


# =============================================================================
# GENERATE SMOOTH FITTED CURVE
# =============================================================================

# Generate evenly spaced concentration values for plotting the fitted curve
c_fit = np.linspace(0, np.max(c_data), 500)

# Evaluate the fitted model over the smooth concentration range
y_fit = model_fixed_span(c_fit, a_fit, n_fit)


# =============================================================================
# CREATE FIGURE
# =============================================================================

# Create either a side-by-side calibration/residual figure or a single fit plot
if PLOT_RESIDUALS:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.6), dpi=200)
else:
    fig, ax1 = plt.subplots(1, 1, figsize=(14, 5.6), dpi=200)


# =============================================================================
# MAIN CALIBRATION FIT PLOT
# =============================================================================

# Plot measured calibration data
ax1.scatter(
    c_data,
    y_data,
    s=12,
    alpha=0.8,
    label="Data"
)

# Plot fitted fixed-SPAN model curve
ax1.plot(
    c_fit,
    y_fit,
    color="darkred",
    linewidth=2.0,
    label="Fit"
)

# Add axis labels
ax1.set_xlabel("Concentration (% v/v)")
ax1.set_ylabel("Normalised Absorbance")

# Add grid and legend for readability
ax1.grid(True, linestyle="--", alpha=0.35)
ax1.legend(frameon=True, loc="lower right")

# Text box summarising fitted parameters and fit quality
param_text = (
    f"a = {a_fit:.6f}\n"
    f"n = {n_fit:.6f}\n"
    f"R² = {r2:.6f}\n"
    f"RMSE = {rmse:.6f}"
)

# Add the parameter text box to the calibration plot
ax1.text(
    0.03, 0.97,
    param_text,
    transform=ax1.transAxes,
    fontsize=18,
    verticalalignment="top",
    bbox=dict(
        boxstyle="round,pad=0.35",
        facecolor="white",
        edgecolor="0.6",
        alpha=0.95
    )
)


# =============================================================================
# RESIDUAL PLOT
# =============================================================================

if PLOT_RESIDUALS:
    # Plot residuals against methane concentration
    ax2.scatter(
        c_data,
        residuals,
        s=12,
        alpha=0.8,
        label="Residuals"
    )

    # Add horizontal zero-residual reference line
    ax2.axhline(
        0,
        color="darkred",
        linewidth=1.6,
        label="Residual = 0"
    )

    # Add axis labels
    ax2.set_xlabel("Concentration (% v/v)")
    ax2.set_ylabel("Residual (NA)")

    # Add grid and legend
    ax2.grid(True, linestyle="--", alpha=0.35)
    ax2.legend(frameon=True, loc="upper right")


# =============================================================================
# FINAL FIGURE FORMATTING
# =============================================================================

# Add an overall figure title only if one has been provided
if FIGURE_TITLE is not None:
    fig.suptitle(FIGURE_TITLE)

# Automatically adjust subplot spacing
fig.tight_layout()

# Leave additional bottom margin for subplot labels "(a)" and "(b)"
fig.subplots_adjust(bottom=0.20)

# Add subplot label for the calibration plot
ax1.text(
    0.5, -0.17, "(a)",
    transform=ax1.transAxes,
    ha="center",
    va="top",
    fontsize=18
)

# Add subplot label for the residual plot, if present
if PLOT_RESIDUALS:
    ax2.text(
        0.5, -0.17, "(b)",
        transform=ax2.transAxes,
        ha="center",
        va="top",
        fontsize=18
    )


# =============================================================================
# SAVE AND DISPLAY FIGURE
# =============================================================================

# Save the figure as a PDF with tight bounding box for report use
fig.savefig(COMBINED_FIGURE_FILE, bbox_inches="tight")

# Display the figure interactively
plt.show()


# =============================================================================
# PRINT NUMERICAL RESULTS
# =============================================================================

# Print fitted model parameters
print("\n=== Fitted Parameters ===")
print(f"Sheet: {SHEET}")
print(f"SPAN (fixed) = {SPAN_FIXED:.5f}")
print(f"a = {a_fit:.6f} ± {a_se:.6f}")
print(f"n = {n_fit:.6f} ± {n_se:.6f}")

# Print goodness-of-fit metrics
print("\n=== Goodness of Fit ===")
print(f"R^2  = {r2:.6f}")
print(f"RMSE = {rmse:.6f}")
print(f"MAE  = {mae:.6f}")

# Print output file location/name
print("\n=== Output File ===")
print(f"Combined figure saved as: {COMBINED_FIGURE_FILE}")
