GroupRS
=====

This repository includes source code of `GroupRS` prototype system. 
GroupRS is a new erasure coding strategy we propose based on Reed-Solomon(RS) code. The core idea of GroupRS is that when the parameters of RS code remain unchanged, we divide the storage nodes of a cluster into multiple distinct groups, and the nodes in each distinct group are independently encoded and decoded. Partitioning nodes can increase the maximum fault tolerance of the cluster, but it will also reduce the parallel repair ability. After our verification, by balancing these two aspects, a reasonable number of distinct groups can be found to make the cluster obtain the best reliability. This prototype system is designed to find the relationship between the optimal number of distinct groups and the characteristics of the cluster. This prototype system can execute GroupRS in the real network environment and cluster, and finally obtain a quantitative metric, which can be used to compare the reliability of the same cluster with different number of distinct groups.

Preparation
----

We implement `GroupRS` on Ubuntu-16.04. So the tutorial as follows is also based on Ubuntu-16.04.

Users can use `apt` to install the required libraries.

 - g++
 - make & cmake
 - nasm
 - libtool & autoconf
 - git

```bash
$  sudo apt install g++ make cmake nasm autoconf libtool git
```

`GroupRS`  is built on ISA-L and Sockpp. Users can install these two libraries manually:

- **Intel®-storage-acceleration-library (ISA-L)**.

  ```bash
  $  git clone https://github.com/intel/isa-l.git
  $  cd isa-l
  $  ./autogen.sh
  $  ./configure; make; sudo make install
  ```

- **Sockpp**

  ```bash
  $  git clone https://github.com/fpagliughi/sockpp.git
  $  cd sockpp
  $  mkdir build ; cd build
  $  cmake ..
  $  make
  $  sudo make install
  $  sudo ldconfig
  ```


## Installation

- Users can install  `GroupRS` via make.

  ```bash
  $  cd grouprs
  $  mkdir build
  $  mkdir build/bin
  $  mkdir build/obj
  $  mkdir test
  $  mkdir test/store
  $  make clean
  $  make all
  ```

## Configuration

- Before running, users should prepare the configure. The example configure are presented in `config/nodes_config.ini`, the meaning of each parameter is described in `config/README.md`.

- Use `dd` to generate a random block in all `helper_nodes` to test.

  ```bash
  $  dd if=/dev/urandom of=test/stdfile bs=1M count=64
  ```

## Run it

- To run the **prototype system**, users should run the `master_node` (at the root directory of repository),

  ```bash
  $  cd grouprs
  $  ./build/bin/node_main 0
  ```

  and run the `helper_nodes` respectively (at the root directory of repository, too).

  ```bash
  $  cd grouprs
  $  ./build/bin/node_main [1/2/3/...]
  ```
