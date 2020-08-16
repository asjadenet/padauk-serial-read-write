# padauk-serial-read-write
Serial port read-write sample, based on the inexpensive Padauk microcontrollers

Serial port speed is ~57600 baud.

## How to use

    git clone https://github.com/asjadenet/padauk-serial-read-write.git
	cd padauk-serial-read-write/
    git submodule update --init --recursive
	make program 
	#or make DEVICE=PFS154 program
	make run
