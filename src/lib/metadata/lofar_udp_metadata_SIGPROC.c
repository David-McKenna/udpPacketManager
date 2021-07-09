
// Internal prototypes
int sigprocStationID(int telescopeId);
__attribute__((unused)) int sigprocMachineID(char *machineName); // Currently not implemented
double sigprocStr2Pointing(char *input);

char* writeKey_SIGPROC(char *buffer, const char *name);
char* writeStr_SIGPROC(char *buffer, const char *name, const char *value);
char* writeInt_SIGPROC(char *buffer, const char *name, int value);
char* writeDouble_SIGPROC(char *buffer, const char *name, double value, int exception);

// Spec doesn't have any longs or floats, define the functions anyway
__attribute__((unused)) char* writeLong_SIGPROC(char *buffer, const char *name, long value);
__attribute__((unused)) char* writeFloat_SIGPROC(char *buffer, const char *name, float value, int exception);


int lofar_udp_metadata_setup_SIGPROC(lofar_udp_metadata *metadata) {

	if (metadata->output.sigproc == NULL) {
		if ((metadata->output.sigproc = calloc(1, sizeof(sigproc_hdr))) == NULL) {
			fprintf(stderr, "ERROR: Failed to allocate memory for sigproc_hdr struct, exiting.\n");
			return -1;
		}
	}
	*(metadata->output.sigproc) = sigproc_hdr_default;

	// Copy over strings
	if (strncpy(metadata->output.sigproc->source_name, metadata->source, META_STR_LEN) != metadata->output.sigproc->source_name) {
		fprintf(stderr, "ERROR: Failed to copy source name to sigproc_hdr, exiting.\n");
		return -1;
	}

	if (strncpy(metadata->output.sigproc->rawdatafile, metadata->rawfile[0], META_STR_LEN) != metadata->output.sigproc->rawdatafile) {
		fprintf(stderr, "ERROR: Failed to copy raw data file to sigproc_hdr, exiting.\n");
		return -1;
	}

	// Copy over assigned metadata
	metadata->output.sigproc->telescope_id = sigprocStationID(metadata->telescope_rsp_id);
	//metadata->output.sigproc->machine_id = sigprocMachineID(metadata->machine);
	metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	metadata->output.sigproc->tsamp = metadata->tsamp;
	metadata->output.sigproc->fch1 = metadata->ftop;
	metadata->output.sigproc->foff = metadata->channel_bw;
	metadata->output.sigproc->nchans = metadata->nchan;

	// Sigproc accepts positive int 32 as a float, unlike other specifications
	metadata->output.sigproc->nbits = metadata->nbit > 0 ? metadata->nbit : -1 * metadata->nbit;

	// Constants for what we produce
	metadata->output.sigproc->nifs = 1;
	metadata->output.sigproc->data_type = 1;

	// Sigproc pointing variables take the form of a double, which is parsed as ddmmss.ss
	metadata->output.sigproc->src_raj = sigprocStr2Pointing(metadata->ra);
	metadata->output.sigproc->src_dej = sigprocStr2Pointing(metadata->dec);

	return 0;
}

int lofar_udp_metadata_update_SIGPROC(lofar_udp_metadata *metadata, int newObs) {

	if (metadata == NULL || metadata->output.sigproc == NULL) {
		fprintf(stderr, "ERROR %s: Input metadata struct is null, exiting.\n", __func__);
		return -1;
	}

	// Sigproc headers are only really meant to be used once at the start of the file, so they don't really change.


	if (newObs) {
		metadata->output.sigproc->tstart = metadata->obs_mjd_start;
	}

	return 0;
}

int lofar_udp_metadata_write_SIGPROC(const sigproc_hdr *hdr, char *headerBuffer, size_t headerLength) {
	if (headerBuffer == NULL) {
		fprintf(stderr, "ERROR: Null buffer provided to %s, exiting.\n", __func__);
	}

	size_t estimatedLength = 300;
	estimatedLength += strlen(hdr->rawdatafile) + strlen(hdr->source_name);

	if (headerLength < estimatedLength) {
		fprintf(stderr, "WARNING: Buffer is short (<%ld chars), we may overflow this buffer. Continuing with caution...\n", estimatedLength);
	}

	char *workingPtr = writeKey_SIGPROC(headerBuffer, "HEADER_START");
	workingPtr = writeInt_SIGPROC(workingPtr, "telescope_id", hdr->telescope_id);
	workingPtr = writeInt_SIGPROC(workingPtr, "machine_id", hdr->machine_id);
	workingPtr = writeInt_SIGPROC(workingPtr, "data_type", hdr->data_type);
	workingPtr = writeStr_SIGPROC(workingPtr, "rawdatafile", hdr->rawdatafile);
	workingPtr = writeStr_SIGPROC(workingPtr, "source_name", hdr->source_name);
	workingPtr = writeInt_SIGPROC(workingPtr, "barycentric", hdr->barycentric);
	workingPtr = writeInt_SIGPROC(workingPtr, "pulsarcentric", hdr->pulsarcentric);
	workingPtr = writeDouble_SIGPROC(workingPtr, "az_start", hdr->az_start, 0);
	workingPtr = writeDouble_SIGPROC(workingPtr, "za_start", hdr->za_start, 0);
	workingPtr = writeDouble_SIGPROC(workingPtr, "src_raj", hdr->src_raj, 0);// Maybe exception? Though ra/dec of  -1 arcsec seems less likely than 0
	workingPtr = writeDouble_SIGPROC(workingPtr, "src_dej", hdr->src_dej, 0);// Maybe exception?
	workingPtr = writeDouble_SIGPROC(workingPtr, "tstart", hdr->tstart, 0);
	workingPtr = writeDouble_SIGPROC(workingPtr, "tsamp", hdr->tsamp, 0);
	workingPtr = writeInt_SIGPROC(workingPtr, "nbits", hdr->nbits);
	//workingPtr = writeInt_SIGPROC(workingPtr, "nsamples", hdr->nsamples);
	workingPtr = writeDouble_SIGPROC(workingPtr, "fch1", hdr->fch1, 0);
	workingPtr = writeDouble_SIGPROC(workingPtr, "foff", hdr->foff, 1);
	workingPtr = writeInt_SIGPROC(workingPtr, "nchans", hdr->nchans);
	workingPtr = writeInt_SIGPROC(workingPtr, "nifs", hdr->nifs);
	workingPtr = writeDouble_SIGPROC(workingPtr, "refdm", hdr->refdm, 0);
	workingPtr = writeDouble_SIGPROC(workingPtr, "period", hdr->period, 0);

	// Don't implement frequency tables for now
	// if (hdr->fchannel != NULL) workingPtr = writeDoubleArray_SIGPROC(workingPtr, header->fchannel);

	workingPtr = writeKey_SIGPROC(workingPtr, "HEADER_END");

	if (workingPtr == NULL) {
		fprintf(stderr, "ERROR: Failed to generate sigproc header exiting.\n");
		return -1;
	}

	return workingPtr - headerBuffer;

}


int sigprocStationID(int telescopeId) {
	switch(telescopeId) {
		// IE613 RSP code
		case 214:
			return 1916;

		default:
			return 0;
	}
}

__attribute__((unused)) int sigprocMachineID(char *machineName) {
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

	int len = strlen(name);

	if (len == 0) {
		fprintf(stderr, "ERROR: Sigproc writer input string is empty, exiting.\n");
		return NULL;
	}

	size_t bufLen = sizeof(int);
	VERBOSE(printf("strlen %s: %d\n", name, len));
	if (memcpy(buffer, &len, bufLen) != buffer) {
		fprintf(stderr, "ERROR: Failed to write string length for string %s, exiting.\n", name);
		return NULL;
	}
	buffer += bufLen;

	if (memcpy(buffer, name, len) != buffer) {
		fprintf(stderr, "ERROR: Failed to write sigproc header string %s to buffer, exiting.\n", name);
		return NULL;
	}

	return buffer + len;
}

char* writeStr_SIGPROC(char *buffer, const char *name, const char *value) {
	if (buffer == NULL) {
		return NULL;
	}

	if (isEmpty(value)) {
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

	if (intNotSet(value)) {
		return buffer;
	}

	if ((buffer = writeKey_SIGPROC(buffer, name)) == NULL) {
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

	if (longNotSet(value)) {
		return buffer;
	}

	if ((buffer = writeKey_SIGPROC(buffer, name)) == NULL) {
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
__attribute__((unused)) char* writeFloat_SIGPROC(char *buffer, const char *name, float value, int exception) {
	if (buffer == NULL) {
		return NULL;
	}

	if (floatNotSet(value, exception)) {
		return buffer;
	}

	if ((buffer = writeKey_SIGPROC(buffer, name)) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(float);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header float %s: %f to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}

char* writeDouble_SIGPROC(char *buffer, const char *name, double value, int exception) {
	if (buffer == NULL) {
		return NULL;
	}

	if (doubleNotSet(value, exception)) {
		return buffer;
	}

	if ((buffer = writeKey_SIGPROC(buffer, name)) == NULL) {
		return NULL;
	}

	size_t chars = sizeof(double);
	if (memcpy(buffer, &value, chars) != buffer) {
		fprintf(stderr, "ERROR: Failed to copy sigproc header int %s: %lf to buffer, exiting.\n", name, value);
		return NULL;
	}

	return buffer + chars;
}