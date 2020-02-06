# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## -[1.20.a1]

### Added

- Report Commands
    -RPAD -get address
    -RPBC - get band channel
    -RPBG - get band group
    -RPFR - get frequency
    -RPID - get osd string
    -RPLF - getlock format
    -RPMD - get mode
    -RPRS - get rssi
    -RPOD - get osd state
    -RPVF - get video format

- Examples in main.py to scan for receivers for connection and version
- Examples in main.py to run reports

### Changed
- Improved exception handling for serial errors that occur when the following events occur
  - Connection error at the serial TTL interface 
  - Connection error at the USB interface

- Modified getter logic to support regex matching and a shared _run_report() method



### Fixed
- Report Commands
  - Improved parsing robustness by using regex's


### TODO
- Improve regex parsing by storing expected limits of values inside clearview_comspecs
- Improve error catching for serial errors by preserving stacktrace while also generating useful error messages
- Improve docstring rendering with rst or md formatting


## [1.20.0] - 2020-02-03 

### Added

- Support of basic ClearView API in concurrent with ClearView 1.20a software version
- Control Commands
  - ADS - Set Receiver Address
  - AN - Set Antenna Mode
  - BC - Set Band Channel 
  - BG - Set Band Group
  - MDL - Show Live Video
  - MDS - Show Spectrum Analyzer
  - MDM - Show User Menu
  - ODE - Show/Enable OSD
  - ODD - Hide/Disable OSD
  - ODP - Set OSD Position (numerically)
  - ID - Set OSD String     
  - RL - Reset ClearView Lock
- Report Commands
  - RPMV - Report Model Version

- File clearview_comspecs.py to specify the serial port configuration and shared protocol specs
- main.py example script
- Unimplemented sections either are broken in the ClearView or have yet to be written here



