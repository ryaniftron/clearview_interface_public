# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2020-02-03 

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



