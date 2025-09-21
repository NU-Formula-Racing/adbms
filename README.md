# NFR adbms
Northwestern Formula Racing's Custom BMS using the ADBMS chipset.

## Overview
An in-depth document elaborating all decisions made for the first itteration (nfr25) is included in `ADBMS Report.pdf`. The hardware is contained in `hardware`, code is contained in `adbms-code`, and analysis in `data-analysis`.

This custom BMS makes use of the ADBMS680B to provide cell voltage and temperature that lives on each of the modules. There is also a main controller board that uses an STM32F405.

## Organization
`main` is considered to be the typical production branch, or what is on the current car.

This is an active repo that contains multiple cars. As a result, develpoment for a certain car happens under the `nfr(year)/` prefix. Following the `nfr(year)/(feature)` template.

The final state of a given year is under `nfr(year)` made by branching off of main after competition for that year. For example, the code currently on nfr25 is under `nfr25`.