#include "temper.h"

#include <libusb-1.0/libusb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int temper_init(temper_t *t){
	const int r=libusb_init(&t->ctx);

	if(r<0)
		fprintf(stderr, "[temper_init] error initializing libusb: %s\n", 
						libusb_error_name(r));
	return r<0;
}

size_t temper_count_sticks(temper_t *t){
	libusb_device **devs;
	ssize_t c=libusb_get_device_list(t->ctx, &devs);
	if(c<0){
			fprintf(stderr, "Error listing devices!\n");
			return 0;
	}

	struct libusb_device_descriptor d;
	
	size_t count=0;
	for(ssize_t i=0;i<c;++i){
		if(libusb_get_device_descriptor(devs[i], &d)<0){
			fprintf(stderr, "Failed getting descriptor for %lu\n", i);
			continue;
		}
	
		if(d.idVendor==TEMPER_VENDOR && d.idProduct==TEMPER_PRODUCT)
				++count;
	}

	libusb_free_device_list(devs, 1);

	return count;
}


static uint8_t temper_prepare_stick(libusb_device_handle *h){
	int r;
	for(int i=0;i<2;++i){
			if((r=libusb_detach_kernel_driver(h, i))){
				fprintf(stderr, "[temper_prepare_stick] failed detaching kernel driver (%s)!\n", 
							libusb_error_name(r));
			/*	
				for(int j=0;j<i;++j)
						libusb_attach_kernel_driver(h, j);
				return 1;
				*/
			}
	}
	
	if((r=libusb_set_configuration(h, 1))){
			fprintf(stderr, "[temper_prepare_stick] failed setting configuration (%s)!\n", 
							libusb_error_name(r));
				
			goto fail_att;
	}

	for(int i=0;i<2;++i){
			if((r=libusb_claim_interface(h, i))){
				fprintf(stderr, "[temper_prepare_stick] failed claiming interface (%s)!\n", 
							libusb_error_name(r));
				
				for(int j=0;j<i;++j)
					libusb_release_interface(h, j);
			
				goto fail_att;
			}
	}

	unsigned char msg[]={0x01, 0x01};

	if((r=libusb_control_transfer(h, 0x21, 0x09, 0x0201, 0x00, msg, 2, TIMEOUT))!=2){
			fprintf(stderr, "[temper_prepare_stick] error at libusb_control_transfer (%s)!\n", 
							libusb_error_name(r));
			goto fail_rel;
	}
	
	unsigned char init_msgs[3][8]={
			{0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00},
			{0x01, 0x82, 0x77, 0x01, 0x00, 0x00, 0x00, 0x00},
			{0x01, 0x86, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00}
	};


	unsigned char buf[8];
	int transferred;

	for(int i=0;i<3;++i){
		if((r=libusb_control_transfer(h, 0x21, 0x09, 0x0200, 0x01, init_msgs[i], 8, TIMEOUT))<0){
			fprintf(stderr, "[temper_prepare_stick] error at libusb_control_transfer WRITE (%s)!\n",
							libusb_error_name(r));
			goto fail_rel;
		}

		//LIBUSB_RECIPIENT_ENDPOINT|LIBUSB_REQUEST_TYPE_STANDARD|LIBUSB_ENDPOINT_IN, 
		
		if((r=libusb_interrupt_transfer(h, 0x82, buf, 8, &transferred, TIMEOUT))!=0){
			fprintf(stderr, "[temper_prepare_stick] error at libusb_interrupt_transfer READ (%s)!\n",
							libusb_error_name(r));
			goto fail_rel;
		}
	}

	if((r=libusb_interrupt_transfer(h, 0x82, buf, 8, &transferred, TIMEOUT))!=0){
		fprintf(stderr, "[temper_prepare_stick] error at libusb_interrupt_transfer READ (%s)!\n",
						libusb_error_name(r));
		goto fail_rel;
	}

	return 0;

fail_rel:
	
	for(int i=0;i<2;++i)
		libusb_release_interface(h, i);

fail_att:

	for(int i=0;i<2;++i)
		libusb_attach_kernel_driver(h, i);
	
	return 1;
}

temper_stick_t *temper_get_stick(temper_t *t, size_t device_num){
	libusb_device **devs;
	ssize_t c=libusb_get_device_list(t->ctx, &devs);
	if(c<0){
			fprintf(stderr, "Error listing devices!\n");
			return NULL;
	}

	struct libusb_device_descriptor d;
	libusb_device_handle *h=NULL;
	
	size_t count=0;
	int r;
	for(ssize_t i=0;i<c;++i){
		if(libusb_get_device_descriptor(devs[i], &d)<0){
			fprintf(stderr, "Failed getting descriptor for %lu\n", i);
			continue;
		}
	
		if(d.idVendor==TEMPER_VENDOR && d.idProduct==TEMPER_PRODUCT){
				if(count==device_num){
					if((r=libusb_open(devs[i], &h)))
						fprintf(stderr, "[temper_get_stick] failed opening stick [%s]!\n", libusb_error_name(r));
					break;
				}else
					++count;
		}
	}

	libusb_free_device_list(devs, 1);

	if(!h){
		fprintf(stderr, "[temper_get_stick] h==NULL\n");
		return NULL;
	}

	if(temper_prepare_stick(h)){
			fprintf(stderr, "[temper_get_stick] error at stick preparation!\n");
			return NULL;
	}

	temper_stick_t *stick=malloc(sizeof(temper_stick_t));
	if(!stick){
			fprintf(stderr, "[temper_get_stick] could not allocate memory!\n");
			return NULL;
	}

	stick->h=h;
	return stick;
}

void temper_stick_free(temper_stick_t *stick){
	if(!stick)
			return;

	for(int i=0;i<2;++i){
		libusb_release_interface(stick->h, i);
		libusb_attach_kernel_driver(stick->h, i);
	}

	libusb_close(stick->h);
	free(stick);
}

uint8_t temper_stick_get_temp(temper_stick_t *stick, int cal, float *temp){
	unsigned char msg[]={0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00};
	
	int r;

	if((r=libusb_control_transfer(stick->h, 0x21, 0x09, 0x0200, 0x01, msg, 8, TIMEOUT))<0){
		fprintf(stderr, "[temper_stick_get_temp] error at libusb_control_transfer WRITE (%s)!\n",
							libusb_error_name(r));
		return 1;
	}		

	for(int i=0;i<8;++i)	
		msg[i]=0;	
	
	int transferred, temp_i;
	if((r=libusb_interrupt_transfer(stick->h, 0x82, msg, 8, &transferred, TIMEOUT))!=0){
			fprintf(stderr, "[temper_stick_get_temp] error at libusb_interrupt_transfer READ (%s)!\n",
							libusb_error_name(r));
			return 1;
	}	

	temp_i=((int) msg[3]) + (((int) msg[2])<<8);
	temp_i+=cal;

	*temp=((float) temp_i)*(125.0/32000.0);
	return 0;
}

void temper_free(temper_t *t){
	if(t->ctx)
			libusb_exit(t->ctx);
}

