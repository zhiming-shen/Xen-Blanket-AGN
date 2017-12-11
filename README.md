# Xen-Blanket-NG

Xen-Blanket is a nested-virtualization solution that works without hardware or software support from underlying cloud providers. With Xen-Blanket, you can get hypervisor-level control by running a second-layer Xen hypervisor within virtual machines (VMs). For more information, checkout the paper: The Xen-Blanket: Virtualize Once, Run Everywhere, in EuroSys 2012.

This is a new version of Xen-Blanket that works with Xen, KVM, and Hyper-V. It is based on Xen 4.2.2, and the Domain-0 kernel is based on Linux 3.4. This is part of the Supercloud project (http://supercloud.cs.cornell.edu/). 

For more information about how to use the source code, refer to the installation scripts in Supercloud-Core (https://github.com/zhiming-shen/Supercloud-core), especially the script https://github.com/zhiming-shen/Supercloud-core/blob/master/setup_xenblanket_1.sh.
