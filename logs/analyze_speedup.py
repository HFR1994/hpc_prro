#!/usr/bin/env python3
"""
Script to extract timing information from HPC log files and calculate speedup.
Log filename format: exec_timings_{placement}_np{processes}_iter{iterations}_pop{population}_feat{features}.log
"""

import re
import pandas as pd
from pathlib import Path


def parse_filename(filename):
    """
    Extract parameters from log filename.

    Args:
        filename: Name of the log file

    Returns:
        Dictionary with extracted parameters or None if parsing fails
    """
    pattern = r'exec_timings_(?P<placement>pack|scatter)_np(?P<np>\d+)_iter(?P<iter>\d+)_pop(?P<pop>\d+)_feat(?P<feat>\d+)\.log'
    match = re.match(pattern, filename)

    if match:
        return {
            'placement': match.group('placement'),
            'np': int(match.group('np')),
            'iterations': int(match.group('iter')),
            'population': int(match.group('pop')),
            'features': int(match.group('feat'))
        }
    return None


def extract_timings(log_path):
    """
    Extract total_time and computation_time from a log file.

    Args:
        log_path: Path to the log file

    Returns:
        Dictionary with total_time and computation_time or None if parsing fails
    """
    timings = {}

    try:
        with open(log_path, 'r') as f:
            content = f.read()

        # Extract Gather time
        gather_match = re.search(r'gather_all:\s+([\d.]+)', content)
        gather_time = 0
        if gather_match:
            gather_time = float(gather_match.group(1))
            timings['gather_time'] = gather_time

        # Extract total_time
        total_match = re.search(r'total_time:\s+([\d.]+)', content)
        if total_match:
            timings['total_time'] = float(total_match.group(1)) - gather_time

        # Extract computation_time
        comp_match = re.search(r'computation_time:\s+([\d.]+)', content)
        if comp_match:
            timings['computation_time'] = float(comp_match.group(1)) - gather_time

        if timings:
            return timings
    except Exception as e:
        print(f"Error reading {log_path}: {e}")

    return None


def process_logs(logs_dir):
    """
    Process all log files in the specified directory.

    Args:
        logs_dir: Path to directory containing log files

    Returns:
        DataFrame with all extracted data
    """
    data = []
    logs_path = Path(logs_dir)

    # Find all .log files
    for log_file in logs_path.glob('trial*/execution*/output/*.log'):
        filename = log_file.name

        # Parse filename
        params = parse_filename(filename)
        if not params:
            print(f"Skipping {filename}: doesn't match expected format")
            continue

        # Extract timings
        timings = extract_timings(log_file)
        if not timings:
            print(f"Warning: Could not extract timings from {filename}")
            continue

        # Combine parameters and timings
        record = {**params, **timings}
        data.append(record)

    # Create DataFrame
    df = pd.DataFrame(data)

    if not df.empty:
        # Sort by parameters for better organization
        df = df.sort_values(['placement', 'population', 'features', 'iterations', 'np'])

    return df


def calculate_speedup(df, reference_np=1, time_column='computation_time'):
    """
    Calculate speedup relative to a reference number of processes.

    Args:
        df: DataFrame with timing data
        reference_np: Reference number of processes for speedup calculation (baseline)
        time_column: Column to use for speedup calculation ('total_time' or 'computation_time')

    Returns:
        DataFrame with speedup columns added
    """
    if df.empty:
        return df

    # Group by configuration (excluding np)
    group_cols = ['placement', 'iterations', 'population', 'features']

    df_with_speedup = df.copy()
    df_with_speedup['speedup'] = None
    df_with_speedup['efficiency'] = None
    df_with_speedup['reference_time'] = None

    for group_values, group_df in df.groupby(group_cols):
        # Find reference time (smallest np or specified reference_np)
        if reference_np in group_df['np'].values:
            ref_time = group_df[group_df['np'] == reference_np][time_column].values[0]
        else:
            # Use smallest np as reference
            min_np = group_df['np'].min()
            ref_time = group_df[group_df['np'] == min_np][time_column].values[0]

        # Calculate speedup for each np in this group
        for idx in group_df.index:
            current_time = df.loc[idx, time_column]
            current_np = df.loc[idx, 'np']

            speedup = ref_time / current_time if current_time > 0 else None
            efficiency = (speedup / current_np * 100) if speedup and current_np > 0 else None

            df_with_speedup.loc[idx, 'speedup'] = speedup
            df_with_speedup.loc[idx, 'efficiency'] = efficiency
            df_with_speedup.loc[idx, 'reference_time'] = ref_time

    return df_with_speedup


def main():
    """Main function to process logs and generate speedup analysis."""

    # Configuration
    logs_dir = '.'  # Change this to your logs directory
    output_csv = 'timing_analysis.csv'
    output_speedup_csv = 'speedup_analysis.csv'

    print(f"Processing logs from: {logs_dir}")

    # Process all log files
    df = process_logs(logs_dir)

    if df.empty:
        print("No data extracted from log files!")
        return

    print(f"\nExtracted data from {len(df)} log files")

    # Save raw timing data
    df.to_csv(output_csv, index=False)
    print(f"Saved timing data to: {output_csv}")

    # Calculate speedup
    df_speedup = calculate_speedup(df, time_column='computation_time')

    # Save speedup analysis
    df_speedup.to_csv(output_speedup_csv, index=False)
    print(f"Saved speedup analysis to: {output_speedup_csv}")

    # Display summary statistics
    print("\n" + "="*80)
    print("SUMMARY STATISTICS")
    print("="*80)

    print("\nTiming Data Summary:")
    print(df[['np', 'total_time', 'computation_time']].describe())

    if 'speedup' in df_speedup.columns:
        print("\nSpeedup Summary:")
        print(df_speedup[['np', 'speedup', 'efficiency']].describe())

        # Show best speedup for each configuration
        print("\n" + "="*80)
        print("BEST SPEEDUP BY CONFIGURATION")
        print("="*80)

        for (placement, pop, feat), group in df_speedup.groupby(['placement', 'population', 'features']):
            best = group.loc[group['speedup'].idxmax()]
            print(f"\n{placement} | pop={pop} | feat={feat}")
            print(f"  Best: np={best['np']} | Speedup={best['speedup']:.2f}x | Efficiency={best['efficiency']:.1f}%")


if __name__ == '__main__':
    main()

