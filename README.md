# clearview_interface_public

Repo that contains scripts to control a ClearView receiver

## Useful Links

[ClearView Website](http://clearview-direct.com/)

[ClearView Updates](http://proteanpaper.com/fwupdate.cgi?comp=iftrontech&manu=2)

## Install

* `$ git clone https://github.com/ryaniftron/clearview_interface_public.git`
* Install python 3.8.1 or later. Recommended to use pyenv
    `https://realpython.com/intro-to-pyenv/`
* `$ cd clearview_interface_public`
* `$ pyenv install 3.8.1`
* `$ pyenv local 3.8.1`
* `$ python --version`
    Expect `3.8.1`
* `python -m pip install pyserial`

## Usage

Check main.py to turn on simulator if you do not have a ClearView. If you do have one connected, turn off the simulator and supply the correct serial port name. 

`$ cd clearview_interface_public`
`$ python src/main.py`
