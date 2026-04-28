"""
Fixed-SPAN methane sensor calibration curve-fitting script.

This script loads processed calibration data from an Excel worksheet, fits the
modified Beer-Lambert response model with a fixed SPAN value, calculates
fit-quality metrics, and generates a side-by-side calibration and residual plot.

Expected input columns:
    - Concentration (%vol)
    - NA_avg

Default input file:
    data/Gas_callibration_data_all_git.xlsx

Default worksheet:
    Feb23_mod

Main output:
    outputs/calibration_fit_and_residuals_side_by_side.pdf

To run:
    python curvefit_fixedSpan_1plot.py

Optional examples:
    python curvefit_fixedSpan_1plot.py --show
    python curvefit_fixedSpan_1plot.py --sheet Feb23_mod
    python curvefit_fixedSpan_1plot.py --span 0.36154
"""

from pathlib import Path
import argparse
import sys

import matplotlib

# Use a non-interactive backend so the script works on a marker's machine,
# GitHub Codespaces, or a terminal without a display.
matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.optimize import curve_fit


# =============================================================================
# DEFAULT SETTINGS
# =============================================================================

BASE_DIR = Path(__file__).resolve().parent
DEFAULT_DATA_FILE = BASE_DIR / "data" / "Gas_callibration_data_all_git.xlsx"
DEFAULT_OUTPUT_DIR = BASE_DIR / "outputs"

DEFAULT_SHEET = "Feb23_mod"

# pandas uses zero-based indexing, so 11 corresponds to Excel row 12.
DEFAULT_HEADER_ROW = 11

# Fixed SPAN value determined experimentally from pure-methane exposure.
DEFAULT_SPAN_FIXED = 0.36154

DEFAULT_OUTPUT_FILE = "calibration_fit_and_residuals_side_by_side.pdf"

REQUIRED_COLUMNS = {"Concentration (%vol)", "NA_avg"}


# =============================================================================
# COMMAND-LINE ARGUMENTS
# =============================================================================

def parse_arguments() -> argparse.Namespace:
    """Parse optional command-line arguments."""
    parser = argparse.ArgumentParser(
        description=(
            "Fit the modified Beer-Lambert methane calibration model "
            "using processed calibration data and a fixed SPAN value."
        )
    )

    parser.add_argument(
        "--file",
        type=Path,
        default=DEFAULT_DATA_FILE,
        help=f"Path to the Excel workbook. Default: {DEFAULT_DATA_FILE}",
    )

    parser.add_argument(
        "--sheet",
        type=str,
        default=DEFAULT_SHEET,
        help=f"Worksheet name to load. Default: {DEFAULT_SHEET}",
    )

    parser.add_argument(
        "--header-row",
        type=int,
        default=DEFAULT_HEADER_ROW,
        help=(
            "Zero-based row index used as the column-header row. "
            f"Default: {DEFAULT_HEADER_ROW}"
        ),
    )

    parser.add_argument(
        "--span",
        type=float,
        default=DEFAULT_SPAN_FIXED,
        help=f"Fixed SPAN value used in the fitted model. Default: {DEFAULT_SPAN_FIXED}",
    )

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"Directory where output files are saved. Default: {DEFAULT_OUTPUT_DIR}",
    )

    parser.add_argument(
        "--output-file",
        type=str,
        default=DEFAULT_OUTPUT_FILE,
        help=f"Output PDF filename. Default: {DEFAULT_OUTPUT_FILE}",
    )

    parser.add_argument(
        "--show",
        action="store_true",
        help="Display the figure interactively after saving.",
    )

    return parser.parse_args()


# =============================================================================
# MODEL DEFINITION
# =============================================================================

def model_fixed_span(c: np.ndarray, a: float, n: float, span_fixed: float) -> np.ndarray:
    """
    Modified Beer-Lambert response model with fixed SPAN.

    Parameters
    ----------
    c : np.ndarray
        Methane concentration in % v/v.
    a : float
        Linearisation coefficient fitted from calibration data.
    n : float
        Exponent coefficient fitted from calibration data.
    span_fixed : float
        Experimentally determined fixed SPAN value.

    Returns
    -------
    np.ndarray
        Predicted normalised absorbance.
    """
    return span_fixed * (1.0 - np.exp(-a * c**n))


def make_curve_fit_model(span_fixed: float):
    """
    Create a two-parameter model function for scipy.curve_fit.

    The SPAN value is fixed, while a and n are fitted.
    """
    def fitted_model(c: np.ndarray, a: float, n: float) -> np.ndarray:
        return model_fixed_span(c, a, n, span_fixed)

    return fitted_model


# =============================================================================
# DATA LOADING
# =============================================================================

def load_calibration_data(
    file_path: Path,
    sheet_name: str,
    header_row: int,
) -> tuple[np.ndarray, np.ndarray]:
    """
    Load and clean processed calibration data from an Excel worksheet.

    Parameters
    ----------
    file_path : Path
        Path to the Excel workbook.
    sheet_name : str
        Worksheet name.
    header_row : int
        Zero-based row index containing column headers.

    Returns
    -------
    tuple[np.ndarray, np.ndarray]
        Concentration data and measured normalised absorbance data.
    """
    if not file_path.exists():
        raise FileNotFoundError(
            f"Input Excel file was not found:\n{file_path}\n\n"
            "Check that the workbook is in the expected data folder, or use "
            "the --file argument to specify its location."
        )

    try:
        df = pd.read_excel(file_path, sheet_name=sheet_name, header=header_row)
    except ValueError as exc:
        raise ValueError(
            f"Could not load worksheet '{sheet_name}' from:\n{file_path}\n\n"
            "Check that the sheet name is correct."
        ) from exc

    # Remove leading/trailing spaces from column names to avoid matching errors.
    df.columns = df.columns.astype(str).str.strip()

    missing = REQUIRED_COLUMNS - set(df.columns)
    if missing:
        raise ValueError(
            f"Missing required columns in sheet '{sheet_name}': {missing}\n"
            f"Available columns are: {list(df.columns)}"
        )

    # Keep only the required columns.
    df = df[["Concentration (%vol)", "NA_avg"]].copy()

    # Convert to numeric. Non-numeric cells become NaN and are removed.
    df["Concentration (%vol)"] = pd.to_numeric(df["Concentration (%vol)"], errors="coerce")
    df["NA_avg"] = pd.to_numeric(df["NA_avg"], errors="coerce")

    # Remove infinities and incomplete rows.
    df = df.replace([np.inf, -np.inf], np.nan).dropna()

    if len(df) < 3:
        raise ValueError(
            "Not enough valid data points for curve fitting. "
            "At least three valid rows are required."
        )

    c_data = df["Concentration (%vol)"].to_numpy(dtype=float)
    y_data = df["NA_avg"].to_numpy(dtype=float)

    # Sort by concentration for clearer plotting.
    sort_idx = np.argsort(c_data)
    c_data = c_data[sort_idx]
    y_data = y_data[sort_idx]

    return c_data, y_data


# =============================================================================
# FITTING AND METRICS
# =============================================================================

def fit_calibration_model(
    c_data: np.ndarray,
    y_data: np.ndarray,
    span_fixed: float,
) -> dict:
    """
    Fit the fixed-SPAN calibration model and calculate fit metrics.

    Parameters
    ----------
    c_data : np.ndarray
        Methane concentration data in % v/v.
    y_data : np.ndarray
        Measured normalised absorbance data.
    span_fixed : float
        Fixed SPAN value.

    Returns
    -------
    dict
        Fitted coefficients, uncertainties, predictions, residuals and metrics.
    """
    fit_model = make_curve_fit_model(span_fixed)

    # Initial parameter guesses for [a, n].
    p0 = [0.3, 0.7]

    # Bounds prevent non-physical negative values.
    bounds = ([0.0, 0.0], [5.0, 2.0])

    popt, pcov = curve_fit(
        fit_model,
        c_data,
        y_data,
        p0=p0,
        bounds=bounds,
        maxfev=50000,
    )

    a_fit, n_fit = popt

    if pcov is None or not np.all(np.isfinite(pcov)):
        a_se, n_se = np.nan, np.nan
    else:
        param_se = np.sqrt(np.diag(pcov))
        a_se, n_se = param_se

    y_pred = fit_model(c_data, a_fit, n_fit)
    residuals = y_data - y_pred

    ss_res = np.sum(residuals**2)
    ss_tot = np.sum((y_data - np.mean(y_data))**2)

    r2 = 1.0 - (ss_res / ss_tot) if ss_tot != 0 else np.nan
    rmse = np.sqrt(np.mean(residuals**2))
    mae = np.mean(np.abs(residuals))

    return {
        "a_fit": a_fit,
        "n_fit": n_fit,
        "a_se": a_se,
        "n_se": n_se,
        "y_pred": y_pred,
        "residuals": residuals,
        "r2": r2,
        "rmse": rmse,
        "mae": mae,
        "fit_model": fit_model,
    }


# =============================================================================
# PLOTTING
# =============================================================================

def configure_plot_style() -> None:
    """Apply report-ready Matplotlib formatting."""
    plt.rcParams.update({
        "font.family": "sans-serif",
        "font.size": 18,
        "axes.labelsize": 20,
        "axes.titlesize": 20,
        "xtick.labelsize": 18,
        "ytick.labelsize": 18,
        "legend.fontsize": 18,
    })


def create_calibration_figure(
    c_data: np.ndarray,
    y_data: np.ndarray,
    fit_results: dict,
    span_fixed: float,
    output_path: Path,
    show_figure: bool = False,
) -> None:
    """
    Create and save the calibration fit and residual figure.

    Parameters
    ----------
    c_data : np.ndarray
        Methane concentration data in % v/v.
    y_data : np.ndarray
        Measured normalised absorbance data.
    fit_results : dict
        Dictionary returned by fit_calibration_model().
    span_fixed : float
        Fixed SPAN value.
    output_path : Path
        Full path where the PDF figure will be saved.
    show_figure : bool
        If True, display the figure interactively after saving.
    """
    configure_plot_style()

    a_fit = fit_results["a_fit"]
    n_fit = fit_results["n_fit"]
    residuals = fit_results["residuals"]
    r2 = fit_results["r2"]
    rmse = fit_results["rmse"]
    fit_model = fit_results["fit_model"]

    c_fit = np.linspace(0.0, np.max(c_data), 500)
    y_fit = fit_model(c_fit, a_fit, n_fit)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.6), dpi=200)

    # Main calibration fit plot.
    ax1.scatter(
        c_data,
        y_data,
        s=12,
        alpha=0.8,
        label="Data",
    )

    ax1.plot(
        c_fit,
        y_fit,
        color="darkred",
        linewidth=2.0,
        label="Fit",
    )

    ax1.set_xlabel("Concentration (% v/v)")
    ax1.set_ylabel("Normalised Absorbance")
    ax1.grid(True, linestyle="--", alpha=0.35)
    ax1.legend(frameon=True, loc="lower right")

    param_text = (
        f"SPAN = {span_fixed:.5f}\n"
        f"a = {a_fit:.6f}\n"
        f"n = {n_fit:.6f}\n"
        f"R² = {r2:.6f}\n"
        f"RMSE = {rmse:.6f}"
    )

    ax1.text(
        0.03,
        0.97,
        param_text,
        transform=ax1.transAxes,
        fontsize=18,
        verticalalignment="top",
        bbox=dict(
            boxstyle="round,pad=0.35",
            facecolor="white",
            edgecolor="0.6",
            alpha=0.95,
        ),
    )

    # Residual plot.
    ax2.scatter(
        c_data,
        residuals,
        s=12,
        alpha=0.8,
        label="Residuals",
    )

    ax2.axhline(
        0.0,
        color="darkred",
        linewidth=1.6,
        label="Residual = 0",
    )

    ax2.set_xlabel("Concentration (% v/v)")
    ax2.set_ylabel("Residual (NA)")
    ax2.grid(True, linestyle="--", alpha=0.35)
    ax2.legend(frameon=True, loc="upper right")

    fig.tight_layout()
    fig.subplots_adjust(bottom=0.20)

    ax1.text(
        0.5,
        -0.17,
        "(a)",
        transform=ax1.transAxes,
        ha="center",
        va="top",
        fontsize=18,
    )

    ax2.text(
        0.5,
        -0.17,
        "(b)",
        transform=ax2.transAxes,
        ha="center",
        va="top",
        fontsize=18,
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_path, bbox_inches="tight")

    if show_figure:
        plt.show()
    else:
        plt.close(fig)


# =============================================================================
# MAIN PROGRAM
# =============================================================================

def main() -> int:
    """Run the full calibration fitting workflow."""
    args = parse_arguments()

    input_file = args.file.resolve()
    output_dir = args.output_dir.resolve()
    output_path = output_dir / args.output_file

    try:
        c_data, y_data = load_calibration_data(
            file_path=input_file,
            sheet_name=args.sheet,
            header_row=args.header_row,
        )

        fit_results = fit_calibration_model(
            c_data=c_data,
            y_data=y_data,
            span_fixed=args.span,
        )

        create_calibration_figure(
            c_data=c_data,
            y_data=y_data,
            fit_results=fit_results,
            span_fixed=args.span,
            output_path=output_path,
            show_figure=args.show,
        )

    except Exception as exc:
        print("\nERROR: Calibration fitting failed.", file=sys.stderr)
        print(str(exc), file=sys.stderr)
        return 1

    print("\n=== Input Data ===")
    print(f"Workbook: {input_file}")
    print(f"Sheet: {args.sheet}")
    print(f"Header row used by pandas: {args.header_row}")
    print(f"Valid data points: {len(c_data)}")

    print("\n=== Fitted Parameters ===")
    print(f"SPAN fixed = {args.span:.5f}")
    print(f"a = {fit_results['a_fit']:.6f} ± {fit_results['a_se']:.6f}")
    print(f"n = {fit_results['n_fit']:.6f} ± {fit_results['n_se']:.6f}")

    print("\n=== Goodness of Fit ===")
    print(f"R^2  = {fit_results['r2']:.6f}")
    print(f"RMSE = {fit_results['rmse']:.6f}")
    print(f"MAE  = {fit_results['mae']:.6f}")

    print("\n=== Output File ===")
    print(f"Combined figure saved as: {output_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
