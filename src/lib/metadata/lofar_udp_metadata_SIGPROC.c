int sigprocStationID(int telescopeId) {
	switch(telescopeId) {
		// IE613 RSP code
		case 214:
			return 1916;

		default:
			return 0;
	}
}

int sigprocMachineID(char *machineName) {
	if (strcmp(machineName, "REALTA_ucc1") == 0) {
		return 1916;
	} else {
		return 0;
	}
}

double sigprocStr2Pointing(char *input) {
	int dd, mm;
	double ss;
	if (sscanf(input, "%d:%d:%lf", &dd, &mm, &ss) != 3) {
		fprintf(stderr, "ERROR: Failed to parse Sigproc RA/Dec pointing, exiting.\n");
		return -1;
	}

	// Flip the sign for minutes / seconds if needed
	int sign = (dd > 0) ? 1 : -1;

	return dd * 10000.0 + sign * mm * 100.0 + sign * ss;
}


char* writeKey_SIGPROC(char *buffer, const char *name) {
	if (buffer == NULL) {
		return NULL;
	}

	// TODO: Assumes value is always not an empty string?
	// TODO: Is caught by writeStr_SIGPROC, though
	int len = strlen(name);
	if(memcpy(buffer, &strlen, sizeof(int)) != buffer) {
		fprintf(stderr, "ERROR: Failed to write string length for stirng %s, exiting.\n", name);
		return NULL;
	}
	buffer += sizeof(int);

	len = sprintf(buffer, "%s", name);
	if (len < 0) {
		fprintf(stderr, "ERROR: Failed to write sigproc header string %s to buffer, exiting.\n", name);
		return NULL;
	}

	return buffer + len;
}

char* writeStr_SIGPROC(char *buffer, const char *name, const char *value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (strlen(value) == 0) {
		return buffer;
	}

	if ((buffer = writeKey_SIGPROC(buffer, name)) == NULL) {
		return NULL;
	}

	if ((buffer = writeKey_SIGPROC(buffer, value)) == NULL) {
		return NULL;
	}

	return buffer;
}

// Wish I had some templates right about now...
char* writeInt_SIGPROC(char *buffer, const char *name, int value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (writeKey_SIGPROC(buffer, name) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(int);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header int %s: %d to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

char* writeLong_SIGPROC(char *buffer, const char *name, long value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (writeKey_SIGPROC(buffer, name) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(long);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header long %s: %ld to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

// Spec doesn't have any floats, define the function anyway
__attribute__((unused)) char* writeFloat_SIGPROC(char *buffer, const char *name, float value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (writeKey_SIGPROC(buffer, name) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(float);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header float %s: %f to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

char* writeDouble_SIGPROC(char *buffer, const char *name, double value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (writeKey_SIGPROC(buffer, name) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(double);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header int %s: %lf to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

int lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata) {

	if (metadata->output.sigproc == NULL) {
		if ((metadata->output.sigproc = calloc(1, sizeof(sigproc_hdr))) == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for sigproc_hdr struct, exiting.\n");
			return -1;
		}
	}

	int returnVal = 0;

	returnVal += (metadata->output.sigproc->rawdatafile != strncpy(metadata->output.sigproc->rawdatafile, metadata->rawfile, DEF_STR_LEN));
	returnVal += (metadata->output.sigproc->source_name != strncpy(metadata->output.sigproc->source_name, metadata->source, META_STR_LEN));

	if (returnVal != 0) {
		fprintf(stderr, "ERROR: Failed to copy data to sigproc_hdr, exiting.\n");
		return -1;
	}

	metadata->output.sigproc->telescope_id = sigprocStationID(metadata->telescope_id);
	metadata->output.sigproc->machine_id = sigprocMachineID(metadata->machine);
	metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	metadata->output.sigproc->tsamp = metadata->tsamp;
	metadata->output.sigproc->fch1 = metadata->ftop;
	metadata->output.sigproc->foff = metadata->channel_bw;
	metadata->output.sigproc->nchans = metadata->nchan;

	metadata->output.sigproc->nifs = 1;
	metadata->output.sigproc->data_type = 1;

	// Sigproc pointing variables take the form of a double, which is parsed as ddmmss.ss
	metadata->output.sigproc->src_raj = sigprocStr2Pointing(metadata->ra);
	metadata->output.sigproc->src_dej = sigprocStr2Pointing(metadata->dec);



	return 0;
}

int lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *metadata) {

	if (metadata->output.sigproc == NULL) {
		if ((metadata->output.sigproc = calloc(1, sizeof(sigproc_hdr))) == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for sigproc_hdr struct, exiting.\n");
			return -1;
		}
	}

	int returnVal = 0;

	returnVal += (metadata->output.sigproc->rawdatafile != strncpy(metadata->output.sigproc->rawdatafile, metadata->rawfile, DEF_STR_LEN));
	returnVal += (metadata->output.sigproc->source_name != strncpy(metadata->output.sigproc->source_name, metadata->source, META_STR_LEN));

	if (returnVal != 0) {
		fprintf(stderr, "ERROR: Failed to copy data to sigproc_hdr, exiting.\n");
		return -1;
	}

	metadata->output.sigproc->telescope_id = sigprocStationID(metadata->telescope_id);
	metadata->output.sigproc->machine_id = sigprocMachineID(metadata->machine);
	metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	metadata->output.sigproc->tsamp = metadata->tsamp;
	metadata->output.sigproc->fch1 = metadata->ftop;
	metadata->output.sigproc->foff = metadata->channel_bw;
	metadata->output.sigproc->nchans = metadata->nchan;

	metadata->output.sigproc->nifs = 1;
	metadata->output.sigproc->data_type = 1;

	// Sigproc pointing variables take the form of a double, which is parsed as ddmmss.ss
	metadata->output.sigproc->src_raj = sigprocStr2Pointing(metadata->ra);
	metadata->output.sigproc->src_dej = sigprocStr2Pointing(metadata->dec);



	return 0;
}

int lofar_udp_metadata_write_SIGPROC(char *headerBuffer, size_t headerLength, sigproc_hdr *headerStruct) {
	if (headerBuffer == NULL) {
		fprintf(stderr, "ERROR: Null buffer provided to %s, exiting.\n", __func__);
	}

	size_t estimatedLength = 300;
	estimatedLength += strlen(headerStruct->rawdatafile) + strlen(headerStruct->source_name);

	if (headerLength < estimatedLength) {
		fprintf(stderr, "WARNING: Buffer is short (<%ld chars), we may overflow this buffer. Continuing with caution...\n", estimatedLength);
	}

	char *workingPtr = writeKey_SIGPROC(headerBuffer, "HEADER_START");
	if (headerStruct->telescope_id != -1) workingPtr = writeInt_SIGPROC(workingPtr, "telescope_id", headerStruct->telescope_id);
	if (headerStruct->machine_id != -1) workingPtr = writeInt_SIGPROC(workingPtr, "machine_id", headerStruct->machine_id);
	if (headerStruct->data_type != -1) workingPtr = writeInt_SIGPROC(workingPtr, "data_type", headerStruct->data_type);
	if (strlen(headerStruct->rawdatafile) > 0) workingPtr = writeStr_SIGPROC(workingPtr, "rawdatafile", headerStruct->rawdatafile);
	if (strlen(headerStruct->source_name) > 0) workingPtr = writeStr_SIGPROC(workingPtr, "source_name", headerStruct->source_name);
	if (headerStruct->barycentric != -1) workingPtr = writeInt_SIGPROC(workingPtr, "barycentric", headerStruct->barycentric);
	if (headerStruct->pulsarcentric != -1) workingPtr = writeInt_SIGPROC(workingPtr, "pulsarcentric", headerStruct->pulsarcentric);
	if (headerStruct->az_start != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "az_start", headerStruct->az_start);
	if (headerStruct->za_start != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "za_start", headerStruct->za_start);
	if (headerStruct->src_raj != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "src_raj", headerStruct->src_raj);
	if (headerStruct->src_dej != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "src_dej", headerStruct->src_dej);
	if (headerStruct->tstart != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "tstart", headerStruct->tstart);
	if (headerStruct->tsamp != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "tsamp", headerStruct->tsamp);
	if (headerStruct->nbits != -1) workingPtr = writeInt_SIGPROC(workingPtr, "nbits", headerStruct->nbits);
	if (headerStruct->nsamples != -1) workingPtr = writeInt_SIGPROC(workingPtr, "nsamples", headerStruct->nsamples);
	if (headerStruct->fch1 != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "fch1", headerStruct->fch1);
	if (headerStruct->foff != 0.0) workingPtr = writeDouble_SIGPROC(workingPtr, "foff", headerStruct->foff);
	if (headerStruct->nchans != -1) workingPtr = writeInt_SIGPROC(workingPtr, "nchans", headerStruct->nchans);
	if (headerStruct->nifs != -1) workingPtr = writeInt_SIGPROC(workingPtr, "nifs", headerStruct->nifs);
	if (headerStruct->refdm != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "refdm", headerStruct->refdm);
	if (headerStruct->period != -1.0) workingPtr = writeDouble_SIGPROC(workingPtr, "period", headerStruct->period);

	// if (headerStruct->fchannel != NULL) workingPtr = writeDoubleArray_SIGPROC(workingPtr, header->fchannel);

	workingPtr = writeKey_SIGPROC(workingPtr, "HEADER_END");

	if (workingPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to generate sigproc header exiting.\n");
		return -1;
	}

	return 0;

}