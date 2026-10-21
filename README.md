# Linux Log Analyzer

A command-line log analysis tool written in C++17 for Linux.

## Features

- Read log files
- Count INFO, WARNING, and ERROR records
- Count unknown-level records and empty lines
- Print an analysis summary to the terminal
- Save the analysis report to a file

## Environment

- Ubuntu 22.04 under WSL2
- G++ 11.4.0
- C++17

## Build

```bash
g++ -std=c++17 -Wall -Wextra -g src/main.cpp -o log_analyzer
```

## Run

```bash
./log_analyzer logs/sample.log
```

## Sample Results

The current sample log contains five records:
- INFO: 2
- WARNING: 1
- ERROR: 2
- Unknown: 0
- Empty lines: 0

## Report
The program writes the analysis report to:
```
reports/analysis_report.txt
```

## Project Purpose
This project practices C++ programming, Linux command-line workflows,
file processing, compilation, debugging, and Git version control.
