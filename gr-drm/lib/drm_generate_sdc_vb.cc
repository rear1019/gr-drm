/* -*- c++ -*- */
/* 
 * Copyright 2012 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <drm_generate_sdc_vb.h>


drm_generate_sdc_vb_sptr
drm_make_generate_sdc_vb (transm_params* tp)
{
	return drm_generate_sdc_vb_sptr (new drm_generate_sdc_vb (tp));
}


drm_generate_sdc_vb::drm_generate_sdc_vb (transm_params* tp)
	: gr_sync_block ("generate_sdc",
		gr_make_io_signature (0, 0, 0),
		gr_make_io_signature (1, 1, sizeof (unsigned char) * tp->sdc().L() ))
{
	d_tp = tp;
}

void
drm_generate_sdc_vb::init_data(unsigned char* data)
{
	unsigned char* data_start = data; // pointer to the beginning of the SDC data
	
	/* set vector to zero as unused data shall be set to zero (see DRM standard) */
	memset(data, 0, d_tp->sdc().L());
	
	/* AFS index */
	enqueue_bits_dec(data, 4, 1); // number of super transmission frames to the next identical SDC block
	
	/* data field 
	 * The data entities consist of a header and a body
	 * header format: length of body (7 bits), version flag (1 bit), data entity type (4 bits) */
	
	
	/* Multiplex description data entity - type 0 */
	
	// header
	enqueue_bits_dec(data, 7, /*n_services*/ 1 * 3); // length of body (3 times the number of services)
	enqueue_bits_dec(data, 1, 0); // reconfiguration flag (current, as reconfiguration is not supported)
	enqueue_bits_dec(data, 4, 0); // data entity type
	
	// body (do this for each service...)
	enqueue_bits_dec(data, 2, d_tp->cfg().msc_prot_level_1()); // if EEP is used, this shall be set to zero (implicitly fulfilled)
	enqueue_bits_dec(data, 2, d_tp->cfg().msc_prot_level_2());
	enqueue_bits_dec(data, 12, d_tp->cfg().n_bytes_A());
	enqueue_bits_dec(data, 12, std::floor(d_tp->msc().L_MUX() / 8));
	
	
	/* Label data entity - type 1 */
	
	// header
	enqueue_bits_dec(data, 7, /*number of utf-8 characters*/ 16); // Maximum number of bytes
	enqueue_bits_dec(data, 1, 0); // unique flag (no meaning)
	enqueue_bits_dec(data, 4, 1); // data entity type
	
	// body
	enqueue_bits_dec(data, 2, 0); // Short ID (denotes the service concerned)
	enqueue_bits_dec(data, 2, 0); // rfu
	unsigned char text[128] = { 0, 1, 0, 0, 0, 0, 1, 1, // C
							   0, 1, 0, 0, 0, 1, 0, 1, // E
							   0, 1, 0, 0, 1, 1, 0, 0, // L 
							   0, 0, 1, 0, 0, 0, 0, 0, // <whitespace>
							   0, 1, 0, 0, 0, 1, 1, 1, // G
							   0, 1, 1, 0, 1, 1, 1, 0, // n
							   0, 1, 1, 1, 0, 1, 0, 1, // u
							   0, 1, 0, 1, 0, 0, 1, 0, // R
							   0, 1, 1, 0, 0, 0, 0, 1, // a
							   0, 1, 1, 0, 0, 1, 0, 0, // d
							   0, 1, 1, 0, 1, 0, 0, 1, // i
							   0, 1, 1, 0, 1, 1, 1, 1, // o
							   0, 0, 1, 0, 0, 0, 0, 0, // <whitespace>
							   0, 1, 0, 0, 0, 1, 0, 0, // D
							   0, 1, 0, 1, 0, 0, 1, 0, // R
							   0, 1, 0, 0, 1, 1, 0, 1  // M
							   };
	enqueue_bits(data, 128, text);
	
	
	/* Time and date information data entity - type 8 */
	
	// header
	enqueue_bits_dec(data, 7, 3); // No local time offset is transmitted
	enqueue_bits_dec(data, 1, 0); // unique flag (no meaning)
	enqueue_bits_dec(data, 4, 8); // data entity type
	
	// body
	enqueue_bits_dec(data, 17, 55110); // arbitrary date in Modified Julian Date format
	enqueue_bits_dec(data, 11, 0); // hours and minutes
	
	
	/* Audio information data entity - type 9 */
	
	// header
	enqueue_bits_dec(data, 7, 2); // length of body
	enqueue_bits_dec(data, 1, 0); // reconfiguration flag (current)
	enqueue_bits_dec(data, 4, 9); // data entity type
	
	// body
	enqueue_bits_dec(data, 2, 0); // short ID (for the service concerned)
	enqueue_bits_dec(data, 2, 0); // stream ID (for the stream that carries the service concerned)
	enqueue_bits_dec(data, 2, 0); // audio coding (AAC)
	enqueue_bits_dec(data, 1, 0); // SBR flag (no SBR used)
	enqueue_bits_dec(data, 2, 0); // audio mode (mono)
	switch(d_tp->cfg().audio_samp_rate()) // audio sample rate
	{
		case 8000:
			enqueue_bits_dec(data, 3, 0);
			break;
		case 12000:
			enqueue_bits_dec(data, 3, 1);
			break;
		case 16000:
			enqueue_bits_dec(data, 3, 2);
			break;
		case 24000:
			enqueue_bits_dec(data, 3, 3);
			break;	
		case 48000:
			enqueue_bits_dec(data, 3, 5);
			break;
		default:
			std::cout << "Invalid audio sample rate!\n";
			break;
	
	}
	enqueue_bits_dec(data, 1, 0); // text flag (no text message carried)
	enqueue_bits_dec(data, 1, 0); // enhancement flag (no enhancement available)
	enqueue_bits_dec(data, 5, 0); // coder field (no MPEG surround)
	enqueue_bits_dec(data, 1, 0); // rfa
	
	
	/* enqueue CRC word */
	enqueue_crc(data_start, d_tp, 16);
					   
}

drm_generate_sdc_vb::~drm_generate_sdc_vb ()
{
}


int
drm_generate_sdc_vb::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
	const unsigned int sdc_length = d_tp->sdc().L();
	unsigned char* data = new unsigned char[sdc_length];
	
	init_data(data);
	memcpy( (void*) output_items[0], (void*) data, (size_t) sizeof(char) * sdc_length );
	delete[] data;
		
	// Tell runtime system how many output items we produced.
	return 1;
}
