obj-m		+= hv_vmbus.o
obj-m	+= hv_utils.o
obj-m	+= hv_storvsc.o
obj-m   += hv_netvsc.o
obj-m	+= hid-hyperv.o

hv_vmbus-y := vmbus_drv.o \
		 hv.o connection.o channel.o \
		 channel_mgmt.o ring_buffer.o
hv_utils-y := hv_util.o hv_kvp.o 
hv_storvsc-y := storvsc_drv.o
hv_netvsc-y := netvsc_drv.o netvsc.o rndis_filter.o
