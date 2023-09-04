# MonteCarloSimulator

A library for ns-3 extending the simulator with the possibility to conduct Monte Carlo simulations. This library was developed for version 3.38 of the ns-3 simulator.

# Building the module
0. Install ns-3.38 as per instructions in the [official documentation](https://www.nsnam.org/documentation/).
1. Clone this repository to `src` of the installed simulator:

```bash
git clone https://github.com/chelminiak/MonteCarloSimulator
```
2. Configure your build with:

```bash
./ns3 configure
```

For more information about the module, see the header file: `model\MonteCarloSimulator.h`.

The example scenario is an implementation of the toy scenario from [Carrascosa, M. and Bellalta, B., 2020. Multi-armed bandits for decentralized AP selection in enterprise WLANs. Computer Communications, 159, pp.108-123](https://www.sciencedirect.com/science/article/pii/S0140366419317980).
