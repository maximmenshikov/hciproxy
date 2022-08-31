# Simple HCI Proxy
A simple HCI <-> UDP proxy. I needed one for experiments, so I've made a trivial one. No major error checks, but works.

## Building

	mkdir -p build && cd build
	cmake -GNinja ..

## Using
	
Run with:

	./hciproxy --port 5712 --dev 0

Now, send UDP packets to host:5712.

## License
MIT
