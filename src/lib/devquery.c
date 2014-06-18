/*   
 * This file is part of cf4ocl (C Framework for OpenCL).
 * 
 * cf4ocl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * cf4ocl is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with cf4ocl. If not, see 
 * <http://www.gnu.org/licenses/>.
 * */
 
/** 
 * @file
 * @brief Functions for querying OpenCL devices.
 * 
 * @author Nuno Fachada
 * @date 2014
 * @copyright [GNU Lesser General Public License version 3 (LGPLv3)](http://www.gnu.org/licenses/lgpl.html)
 * */
 
#include "devquery.h"

/* Implementation of cl4_devquery_format() function for outputting
 * unsigned integers. */
static gchar* cl4_devquery_format_uint(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
		
	g_snprintf(out, size, "%d %s", *((cl_uint*) info->value), units);
	return out;
	
}

/* Implementation of cl4_devquery_format() function for outputting
 * device information as a hexadecimal number. */
static gchar* cl4_devquery_format_hex(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
		
	GString* str = g_string_new("0x");
	gboolean start = FALSE;
	gchar val;
	
	for (gint i = info->size - 1; i >= 0 ; i--) {
		val = ((cl_char*) info->value)[i];
		if (val) start = TRUE;
		if (start)
			g_string_append_printf(str, "%.2x", val);
	}
	if (units && units[0])
		g_string_append_printf(str, " %s", units);
		
	g_snprintf(out, size, "%s", str->str);
	g_string_free(str, TRUE);
	return out;
	
}

/* Implementation of cl4_devquery_format() function for outputting
 * size_t unsigned integers. */
static gchar* cl4_devquery_format_sizet(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
		
	g_snprintf(out, size, "%ld %s", (gulong) *((size_t*) info->value), units);
	return out;
	
}

/* Helper macro for format functions outputting values in bytes. */
#define cl4_devquery_format_bytes(spec) \
	if (bytes < 1024) \
		g_snprintf(out, size, "%" spec " bytes", bytes); \
	else if (bytes < 1048576) \
		g_snprintf(out, size, "%.1lf KiB (%" spec " bytes)", \
			bytes / 1024.0, bytes); \
	else if (bytes < 1073741824) \
		g_snprintf(out, size, "%.1lf MiB (%" spec " bytes)", \
			bytes / (1024.0 * 1024), bytes); \
	else \
		g_snprintf(out, size, "%.1lf GiB (%" spec " bytes)", \
			bytes / (1024.0 * 1024 * 1024), bytes);

/* Implementation of cl4_devquery_format() function for outputting
 * unsigned long integers which represent bytes. */
static gchar* cl4_devquery_format_ulongbytes(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_ulong bytes = *((cl_ulong*) info->value);
	cl4_devquery_format_bytes("ld");
	return out;
	
}

/* Implementation of cl4_devquery_format() function for outputting
 * unsigned integers which represent bytes. */
static gchar* cl4_devquery_format_uintbytes(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_uint bytes = *((cl_uint*) info->value);
	cl4_devquery_format_bytes("d");
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * unsigned size_t integers which represent bytes. */
static gchar* cl4_devquery_format_sizetbytes(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_ulong bytes = *((size_t*) info->value);
	cl4_devquery_format_bytes("ld");
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * a vector of size_t integers. */
static gchar* cl4_devquery_format_sizetvec(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	GString* str = g_string_new("(");
	size_t* vec = (size_t*) info->value;
	guint count = info->size / sizeof(gsize);

	for (guint i = 0; i < count; i++) {
		if (i > 0) g_string_append(str, ", ");
		g_string_append_printf(str, "%ld", (gulong) vec[i]);
	}
	
	g_string_append(str, ")");

	g_snprintf(out, size, "%s", str->str);
	g_string_free(str, TRUE);
	return out;
}

/* Implementation of cl4_devquery_format() function for outputting
 * boolean values as a "Yes" or "No" string. */
static gchar* cl4_devquery_format_yesno(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	g_snprintf(out, size, "%s", *((cl_bool*) info->value) ? "Yes" : "No");
	return out;
	
}

/* Implementation of cl4_devquery_format() function for outputting
 * strings. */
static gchar* cl4_devquery_format_char(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	g_snprintf(out, size, "%s %s", (gchar*) info->value, units);
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * memory addresses. */
static gchar* cl4_devquery_format_ptr(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	g_snprintf(out, size, "%p", *((void**) info->value));
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * a string representing a device type. */
static gchar* cl4_devquery_format_type(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	g_snprintf(out, size, "%s", 
		cl4_devquery_type2str(*((cl_device_type*) info->value)));
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * the device floating-point (FP) configuration for a FP type. */
static gchar* cl4_devquery_format_fpconfig(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_device_fp_config fpc = *((cl_device_fp_config*) info->value);
	g_snprintf(out, size, "%s%s%s%s%s%s%s", 
		fpc & CL_FP_DENORM ? "DENORM " : "",
		fpc & CL_FP_INF_NAN  ? "INF_NAN " : "",
		fpc & CL_FP_ROUND_TO_NEAREST ? "ROUND_TO_NEAREST " : "",
		fpc & CL_FP_ROUND_TO_ZERO ? "ROUND_TO_ZERO " : "",
		fpc & CL_FP_ROUND_TO_INF ? "ROUND_TO_INF " : "",
		fpc & CL_FP_FMA ? "FMA " : "",
		fpc & CL_FP_SOFT_FLOAT ? "SOFT_FLOAT" : "");
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting
 * the device execution capabilities. */
static gchar* cl4_devquery_format_execcap(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {

	units = units;
	cl_device_exec_capabilities exc = 
		*((cl_device_exec_capabilities*) info->value);
	g_snprintf(out, size, "%s%s", 
		exc & CL_EXEC_KERNEL ? " KERNEL" : "",
		exc & CL_EXEC_NATIVE_KERNEL ? " NATIVE_KERNEL" : "");
	return out;
}

/* Implementation of cl4_devquery_format() function for outputting
 * a local memory type. */
static gchar* cl4_devquery_format_locmemtype(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_device_local_mem_type lmt = 
		*((cl_device_local_mem_type*) info->value);
	g_snprintf(out, size, "%s%s%s", 
		lmt & CL_LOCAL ? "LOCAL" : "",
		lmt & CL_GLOBAL ? "GLOBAL" : "",
		lmt & CL_NONE ? "NONE" : "");
	return out;
}

/* Implementation of cl4_devquery_format() function for outputting the
 * partition properties of a device. */
static gchar* cl4_devquery_format_partprop(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_device_partition_property* pp = 
		(cl_device_partition_property*) info->value;
	GString* str = g_string_new("");
	guint count = info->size / sizeof(cl_device_partition_property);
	for (guint i = 0; i < count; i++) {
		switch (pp[i]) {
			case CL_DEVICE_PARTITION_EQUALLY:
				g_string_append_printf(str, "EQUALLY ");
				break;
			case CL_DEVICE_PARTITION_BY_COUNTS:
				g_string_append_printf(str, "BY_COUNTS ");
				break;
			case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN:
				g_string_append_printf(str, "BY_AFFINITY_DOMAIN ");
				break;
			case CL_DEVICE_PARTITION_EQUALLY_EXT:
				g_string_append_printf(str, "EQUALLY_EXT ");
				break;
			case CL_DEVICE_PARTITION_BY_COUNTS_EXT:
				g_string_append_printf(str, "BY_COUNTS_EXT ");
				break;
			case CL_DEVICE_PARTITION_BY_NAMES_EXT:
				g_string_append_printf(str, "BY_NAMES_EXT ");
				break;
			case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT:
				g_string_append_printf(str, "BY_AFFINITY_DOMAIN_EXT ");
				break;
			default:
				g_string_append_printf(str, "UNKNOWN(0x%lx) ", pp[i]);
		}
	}
	g_snprintf(out, size, "%s", str->str);
	g_string_free(str, TRUE);
	return out;
	
}

/* Implementation of cl4_devquery_format() function for outputting the
 * supported affinity domains for partitioning a device using 
 * CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN. */
static gchar* cl4_devquery_format_affdom(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_device_affinity_domain ad = 
		*((cl_device_affinity_domain*) info->value);
	g_snprintf(out, size, "%s%s%s%s%s%s", 
		ad & CL_DEVICE_AFFINITY_DOMAIN_NUMA ? "NUMA " : "",
		ad & CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE ? "L4_CACHE " : "",
		ad & CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE ? "L3_CACHE " : "",
		ad & CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE ? "L2_CACHE " : "",
		ad & CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE ? "L1_CACHE " : "",
		ad & CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE ? 
			"NEXT_PARTITIONABLE " : "");
	return out;

}

/* Implementation of cl4_devquery_format() function for outputting the
 * cache type of a device. */
static gchar* cl4_devquery_format_cachetype(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_device_mem_cache_type mct = 
		*((cl_device_mem_cache_type*) info->value);
	g_snprintf(out, size, "%s%s%s", 
		mct & CL_READ_ONLY_CACHE ? "READ_ONLY" : "",
		mct & CL_READ_WRITE_CACHE ? "READ_WRITE" : "",
		mct & CL_NONE ? "NONE" : "");
	return out;
}

/* Implementation of cl4_devquery_format() function for outputting the
 * queue properties of a device. */
static gchar* cl4_devquery_format_queueprop(CL4DeviceInfoValue* info, 
	gchar* out, guint size, const gchar const* units) {
	
	units = units;
	cl_command_queue_properties qp = 
		*((cl_command_queue_properties*) info->value);
	g_snprintf(out, size, "%s%s", 
		qp & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ? "OUT_OF_ORDER_EXEC_MODE_ENABLE " : "",
		qp & CL_QUEUE_PROFILING_ENABLE ? "PROFILING_ENABLE " : "");
	return out;

}

//~ /* Implementation of cl4_devquery_format() function for debugging
 //~ * purposes. */
//~ static gchar* cl4_devquery_format_testsize(CL4DeviceInfoValue* info, 
	//~ gchar* out, guint size, const gchar const* units) {
	//~ 
	//~ g_snprintf(out, size, "%ld %s", (gulong) info->size, units);
	//~ return out;
//~ 
//~ }

/** 
 * @brief Size of parameter information map. 
 * */
const gint cl4_devquery_info_map_size = 96;

/** 
 * @brief Map of parameter name strings to respective cl_device_info 
 * bitfields, long description string, format output function and a
 * units suffix. 
 * */
const CL4DevQueryMap cl4_devquery_info_map[] = {

	{"address_bits", CL_DEVICE_ADDRESS_BITS, 
		"Address space size in bits", 
		cl4_devquery_format_uint, "bits"},
	{"available", CL_DEVICE_AVAILABLE, 
		"Is device available", 
		cl4_devquery_format_yesno, ""},
	{"board_name_amd", CL_DEVICE_BOARD_NAME_AMD, 
		"Name of the GPU board and model of the specific device", 
		cl4_devquery_format_char, ""},
	{"built_in_kernels", CL_DEVICE_BUILT_IN_KERNELS, 
		"Device built-in kernels", 
		cl4_devquery_format_char, ""},
	{"compiler_available", CL_DEVICE_COMPILER_AVAILABLE, 
		"Is a compiler available for device", 
		cl4_devquery_format_yesno, ""},
	{"compute_capability_major_nv", CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, 
		"Major revision number that defines the CUDA compute capability of the device", 
		cl4_devquery_format_uint, ""},
	{"compute_capability_minor_nv", CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, 
		"Minor revision number that defines the CUDA compute capability of the device", 
		cl4_devquery_format_uint, ""},
	{"double_fp_config", CL_DEVICE_DOUBLE_FP_CONFIG, 
		"Floating-point device configuration (double)", 
		cl4_devquery_format_fpconfig, ""},
	{"driver_version", CL_DRIVER_VERSION, 
		"Driver version", 
		cl4_devquery_format_char, ""},
	{"endian_little", CL_DEVICE_ENDIAN_LITTLE, 
		"Is device little endian", 
		cl4_devquery_format_yesno, ""},
	{"error_correction_support", CL_DEVICE_ERROR_CORRECTION_SUPPORT, 
		"Error correction support", 
		cl4_devquery_format_yesno, ""},
	{"execution_capabilities", CL_DEVICE_EXECUTION_CAPABILITIES, 
		"Execution capabilities", 
		cl4_devquery_format_execcap, ""},
	{"extensions", CL_DEVICE_EXTENSIONS, 
		"Extensions", 
		cl4_devquery_format_char, ""},
	/* The parameter bellow has in fact a length of 16 bytes,
	 * but due to the lack of documentation, only the first half 
	 * of it is shown for now. */
	{"global_free_memory_amd", CL_DEVICE_GLOBAL_FREE_MEMORY_AMD, 
		"Free device memory", 
		cl4_devquery_format_ulongbytes, ""},
	{"global_mem_cache_size", CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, 
		"Global mem. cache size", 
		cl4_devquery_format_ulongbytes, ""},
	{"global_mem_cache_type", CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, 
		"Global mem. cache type", 
		cl4_devquery_format_cachetype, ""},
	{"global_mem_cacheline_size", CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, 
		"Global mem. cache line size", 
		cl4_devquery_format_uintbytes, ""},
	{"global_mem_channel_bank_width_amd", CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD, 
		"Global mem. channel bank width", 
		cl4_devquery_format_uint, ""},
	{"global_mem_channel_banks_amd", CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD, 
		"Global mem. channel banks", 
		cl4_devquery_format_uint, ""},
	{"global_mem_channels_amd", CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD, 
		"Global mem. channels", 
		cl4_devquery_format_uint, ""},
	{"global_mem_size", CL_DEVICE_GLOBAL_MEM_SIZE, 
		"Global mem. size", 
		cl4_devquery_format_ulongbytes, ""},
	{"gpu_overlap_nv", CL_DEVICE_GPU_OVERLAP_NV, 
		"Can device concurrently copy memory between host and device while executing a kernel", 
		cl4_devquery_format_yesno, ""},
	{"half_fp_config", CL_DEVICE_HALF_FP_CONFIG, 
		"Floating-point device configuration (half)", 
		cl4_devquery_format_fpconfig, ""},
	{"host_unified_memory", CL_DEVICE_HOST_UNIFIED_MEMORY, 
		"Host unified memory subsystem", 
		cl4_devquery_format_yesno, ""},
	{"image2d_max_height", CL_DEVICE_IMAGE2D_MAX_HEIGHT, 
		"Max. height of 2D image (pixels)", 
		cl4_devquery_format_sizet, "px"},
	{"image2d_max_width", CL_DEVICE_IMAGE2D_MAX_WIDTH, 
		"Max. width of 1D/2D image (pixels)", 
		cl4_devquery_format_sizet, "px"},
	{"image3d_max_depth", CL_DEVICE_IMAGE3D_MAX_DEPTH, 
		"Max. depth of 3D image (pixels)", 
		cl4_devquery_format_sizet, "px"},
	{"image3d_max_height", CL_DEVICE_IMAGE3D_MAX_HEIGHT, 
		"Max. height of 3D image (pixels)", 
		cl4_devquery_format_sizet, "px"},
	{"image3d_max_width", CL_DEVICE_IMAGE3D_MAX_WIDTH, 
		"Max. width of 3D image (pixels)", 
		cl4_devquery_format_sizet, "px"},
	{"image_max_buffer_size", CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, 
		"Max. pixels for 1D image from buffer object", 
		cl4_devquery_format_sizet, "px"},
	{"image_max_array_size", CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, 
		"Max. images in a 1D or 2D image array", 
		cl4_devquery_format_sizet, "images"},
	{"image_support", CL_DEVICE_IMAGE_SUPPORT, 
		"Image support", 
		cl4_devquery_format_yesno, ""},
	{"integrated_memory_nv", CL_DEVICE_INTEGRATED_MEMORY_NV, 
		"Is device integrated with the memory subsystem?", 
		cl4_devquery_format_yesno, ""},
	{"kernel_exec_timeout_nv", CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV, 
		"Is there a limit for kernels executed on device?", 
		cl4_devquery_format_yesno, ""},
	{"linker_available", CL_DEVICE_LINKER_AVAILABLE, 
		"Linker available", 
		cl4_devquery_format_yesno, ""},
	{"local_mem_banks_amd", CL_DEVICE_LOCAL_MEM_BANKS_AMD, 
		"Local mem. banks", 
		cl4_devquery_format_uint, ""},
	{"local_mem_size", CL_DEVICE_LOCAL_MEM_SIZE, 
		"Local mem. size", 
		cl4_devquery_format_ulongbytes, ""},
	{"local_mem_size_per_compute_unit_amd", CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD, 
		"Local mem. size per compute unit", 
		cl4_devquery_format_uintbytes, ""},
	{"local_mem_type", CL_DEVICE_LOCAL_MEM_TYPE, 
		"Local mem. type", 
		cl4_devquery_format_locmemtype, ""},
	{"max_atomic_counters_ext", CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT, 
		"Max. atomic counters", 
		cl4_devquery_format_uint, ""},
	{"max_clock_frequency", CL_DEVICE_MAX_CLOCK_FREQUENCY, 
		"Max. clock frequency (MHz)", 
		cl4_devquery_format_uint, "MHz"},
	{"max_compute_units", CL_DEVICE_MAX_COMPUTE_UNITS, 
		"Number of compute units in device", 
		cl4_devquery_format_uint, ""},
	{"max_constant_args", CL_DEVICE_MAX_CONSTANT_ARGS, 
		"Max. number of __constant args in kernel", 
		cl4_devquery_format_uint, ""},
	{"max_constant_buffer_size", CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, 
		"Max. size in bytes of a constant buffer allocation", 
		cl4_devquery_format_ulongbytes, ""},
	{"max_mem_alloc_size", CL_DEVICE_MAX_MEM_ALLOC_SIZE, 
		"Max. size of memory object allocation in bytes", 
		cl4_devquery_format_ulongbytes, ""},
	{"max_parameter_size", CL_DEVICE_MAX_PARAMETER_SIZE, 
		"Max. size in bytes of the arguments that can be passed to a kernel", 
		cl4_devquery_format_sizetbytes, ""},
	{"max_read_image_args", CL_DEVICE_MAX_READ_IMAGE_ARGS, 
		"Max. number of simultaneous image objects that can be read by a kernel", 
		cl4_devquery_format_uint, "images"},
	{"max_samplers", CL_DEVICE_MAX_SAMPLERS, 
		"Max. samplers that can be used in kernel", 
		cl4_devquery_format_uint, "samplers"},
	{"max_work_group_size", CL_DEVICE_MAX_WORK_GROUP_SIZE, 
		"Max. work-items in work-group executing a kernel on a single compute unit, using the data parallel execution model", 
		cl4_devquery_format_sizet, "work-items"},
	{"max_work_item_dimensions", CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 
		"Max. dims that specify the global and local work-item IDs used by the data parallel execution model", 
		cl4_devquery_format_uint, ""},
	{"max_work_item_sizes", CL_DEVICE_MAX_WORK_ITEM_SIZES, 
		"Max. work-items in each dimension of work-group", 
		cl4_devquery_format_sizetvec, ""},
	{"max_write_image_args", CL_DEVICE_MAX_WRITE_IMAGE_ARGS, 
		"Max. simultaneous image objects that can be written to by a kernel", 
		cl4_devquery_format_uint, "images"},
	{"mem_base_addr_align", CL_DEVICE_MEM_BASE_ADDR_ALIGN, 
		"Size in bits of the largest OpenCL built-in data type supported by the device", 
		cl4_devquery_format_uint, "bits"},
	{"min_data_type_align_size", CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, 
		"Smallest alignment which can be used for any data type (deprecated in OpenCL 1.2)", 
		cl4_devquery_format_uintbytes, ""},
	{"name", CL_DEVICE_NAME, 
		"Name of device", 
		cl4_devquery_format_char, ""},
	{"native_vector_width_char", CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, 
		"Native ISA char vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_double", CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, 
		"Native ISA double vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_float", CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, 
		"Native ISA float vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_half", CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, 
		"Native ISA half vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_int", CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, 
		"Native ISA int vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_long", CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, 
		"Native ISA long vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"native_vector_width_short", CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, 
		"Native ISA short vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"opencl_c_version", CL_DEVICE_OPENCL_C_VERSION, 
		"Highest OpenCL C version supported by the device compiler", 
		cl4_devquery_format_char, ""},
	{"parent_device", CL_DEVICE_PARENT_DEVICE, 
		"The cl_device_id of the parent device to which the sub-device belongs", 
		cl4_devquery_format_ptr, ""},
	{"partition_max_sub_devices", CL_DEVICE_PARTITION_MAX_SUB_DEVICES, 
		"Max. sub-devices that can be created when device is partitioned", 
		cl4_devquery_format_uint, "devices"},
	{"partition_properties", CL_DEVICE_PARTITION_PROPERTIES, 
		"Partition types supported by device", 
		cl4_devquery_format_partprop, ""},
	{"partition_affinity_domain", CL_DEVICE_PARTITION_AFFINITY_DOMAIN, 
		"Supported affinity domains for partitioning the device using CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN", 
		cl4_devquery_format_affdom, ""},
	{"partition_type", CL_DEVICE_PARTITION_TYPE, 
		"Properties specified in clCreateSubDevices if device is a subdevice", 
		cl4_devquery_format_uint, ""},
	{"platform", CL_DEVICE_PLATFORM, 
		"The platform associated with device", 
		cl4_devquery_format_ptr, ""},
	{"preferred_interop_user_sync", CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, 
		"'Yes' if device prefers user to be responsible for sync. when sharing memory objects between OpenCL and other APIs, 'No' if device has a performant path for performing such sync.", 
		cl4_devquery_format_yesno, ""},
	{"preferred_vector_width_char", CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, 
		"Preferred ISA char vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_double", CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, 
		"Preferred ISA double vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_float", CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, 
		"Preferred ISA float vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_half", CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, 
		"Preferred ISA half vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_int", CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, 
		"Preferred ISA int vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_long", CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, 
		"Preferred ISA long vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"preferred_vector_width_short", CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, 
		"Preferred ISA short vector width (number of scalar elements that can be stored in the vector)", 
		cl4_devquery_format_uint, ""},
	{"printf_buffer_size", CL_DEVICE_PRINTF_BUFFER_SIZE, 
		"Max. size of internal buffer that holds the output of printf calls from kernel", 
		cl4_devquery_format_sizetbytes, ""},
	{"profile", CL_DEVICE_PROFILE, 
		"Profile name supported by the device (FULL or EMBEDDED)", 
		cl4_devquery_format_char, ""},
	{"profiling_timer_offset_amd", CL_DEVICE_PROFILING_TIMER_OFFSET_AMD, 
		"Offset between event timestamps in nanoseconds", 
		cl4_devquery_format_sizet, "ns"},
	{"profiling_timer_resolution", CL_DEVICE_PROFILING_TIMER_RESOLUTION, 
		"Resolution of device timer in nanoseconds", 
		cl4_devquery_format_sizet, "ns"},
	{"queue_properties", CL_DEVICE_QUEUE_PROPERTIES, 
		"Command-queue properties supported by device", 
		cl4_devquery_format_queueprop, ""},
	{"reference_count", CL_DEVICE_REFERENCE_COUNT, 
		"Device reference count", 
		cl4_devquery_format_uint, ""},
	{"registers_per_block_nv", CL_DEVICE_REGISTERS_PER_BLOCK_NV, 
		"Maximum number of 32-bit registers available to a work-group", 
		cl4_devquery_format_uint, ""},
	{"simd_instruction_width_amd", CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD, 
		"SIMD instruction width", 
		cl4_devquery_format_uint, ""},
	{"simd_per_compute_unit_amd", CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD, 
		"SIMD per compute unit", 
		cl4_devquery_format_uint, ""},
	{"simd_width_amd", CL_DEVICE_SIMD_WIDTH_AMD, 
		"SIMD width", 
		cl4_devquery_format_uint, ""},
	{"single_fp_config", CL_DEVICE_SINGLE_FP_CONFIG, 
		"Floating-point device configuration (single)", 
		cl4_devquery_format_fpconfig, ""},
	{"thread_trace_supported_amd", CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD, 
		"Is thread trace supported", 
		cl4_devquery_format_yesno, ""},
	{"topology_amd", CL_DEVICE_TOPOLOGY_AMD, 
		"Description of the topology used to connect the device to the host", 
		cl4_devquery_format_hex, ""},
	{"type", CL_DEVICE_TYPE, 
		"Type of OpenCL device", 
		cl4_devquery_format_type, ""},
	{"vendor", CL_DEVICE_VENDOR, 
		"Vendor of OpenCL device", 
		cl4_devquery_format_char, ""},
	{"vendor_id", CL_DEVICE_VENDOR_ID, 
		"Unique device vendor identifier", 
		cl4_devquery_format_hex, ""},
	{"version", CL_DEVICE_VERSION, 
		"OpenCL software driver version", 
		cl4_devquery_format_char, ""},
	{"warp_size_nv", CL_DEVICE_WARP_SIZE_NV, 
		"Warp size in work-items", 
		cl4_devquery_format_uint, ""},
	{"wavefront_width_amd", CL_DEVICE_WAVEFRONT_WIDTH_AMD, 
		"Wavefront width", 
		cl4_devquery_format_uint, ""}		
		
};

/**
 * @brief Get a final device info prefix in the same format as 
 * kept in the cl4_devquery_info_map.
 * 
 * @param prefix Raw device information prefix. Several forms are 
 * accepted. For example, for CL_DEVICE_ENDIAN_LITTLE, strings such as
 * "CL_DEVICE_ENDIAN_LITTLE", "ENDIAN_LITTLE" or "endian_little" are
 * accepted.
 * @return A final device info prefix in the same format as 
 * kept in the cl4_devquery_info_map.
 * */
static gchar* cl4_devquery_get_prefix_final(gchar* prefix) {
	
	/* Make sure prefix is not NULL. */
	g_return_val_if_fail(prefix != NULL, 0);
	
	/* Auxiliary string variables. */
	gchar* str_lower;
	gchar* str_aux;
	gchar* str_final;
	
	/* Make string lower-case. */
	str_lower = g_ascii_strdown(prefix, -1);
	
	/* Remove possible cl_device or cl_ prefix */
	if (g_str_has_prefix(str_lower, "cl_device_")) {
		str_aux = str_lower + strlen("cl_device_");
	} else if (g_str_has_prefix(str_lower, "cl_")) {
		str_aux = str_lower + strlen("cl_");
	} else {
		str_aux = str_lower;
	}
	
	/* Create a new string for returning. */
	str_final = g_strdup(str_aux);

	/* Free lowercase string. */
	g_free(str_lower);
	
	/* Return result */
	return str_final;
	
}

/**
 * @brief Return the index of the device information map object of the
 * given parameter name.
 * 
 * @param A parameter name, in the format stored in the 
 * cl4_devquery_info_map array.
 * @return Index of the device information map object of the given 
 * parameter name, or -1 if device information map is not found.
 * */
static gint cl4_devquery_get_index(gchar* name) {

	/* Make sure name is not NULL. */
	g_return_val_if_fail(name != NULL, 0);
	
	/* Search indexes. */
	gint idx_middle;
	guint idx_start, idx_end;
	
	/* String comparison result. */
	gint cmp_res;
	
	/* Found info flag. */
	gboolean found = FALSE;
	
	/* Size of name. */
	gint len_name = strlen(name);

	/* Binary search. */
	idx_start = 0;
	idx_end = cl4_devquery_info_map_size - 1;
	while (idx_end >= idx_start) {
		idx_middle = (idx_start + idx_end) / 2;
		cmp_res = g_ascii_strncasecmp(
			name, cl4_devquery_info_map[idx_middle].param_name, len_name);
		if (cmp_res == 0) {
			found = TRUE;
			break;
		}
		if (cmp_res > 0)
			idx_start = idx_middle + 1;
		else 
			idx_end = idx_middle - 1;
	}
	
	/* Return result */
	return found ? idx_middle : -1;
}

/**
 * @brief Return a cl_device_info object given its name.
 * 
 * @param name Name of cl_device_info object. Several forms are 
 * accepted. For example, for CL_DEVICE_ENDIAN_LITTLE, strings such as
 * "CL_DEVICE_ENDIAN_LITTLE", "ENDIAN_LITTLE" or "endian_little" are
 * accepted.
 * @return A cl_device_info object given its name or 0 if no suitable
 * cl_device_info is found for the given name.
 * */
cl_device_info cl4_devquery_name(gchar* name) {

	/* Make sure name is not NULL. */
	g_return_val_if_fail(name != NULL, 0);

	/* Final name */
	gchar* name_final;
	
	/* Index of device info. */
	gint idx;
	
	/* Get final name. */
	name_final = cl4_devquery_get_prefix_final(name);
	
	/* Get index of cl_device_info given its final name. */
	idx = cl4_devquery_get_index(name_final);
	
	/* Release memory holding the final name. */
	g_free(name_final);
	
	/* Return the cl_device_info object if found, or 0 otherwise. */
	if (idx >= 0)
		return cl4_devquery_info_map[idx].device_info;
	else
		return 0;
	
}

/**
 * @brief Get a list of device information parameters which have the 
 * given prefix.
 * 
 * @param prefix Device information parameter prefix. Can be in lower
 * or uppercase, and start with "cl_device_" or not.
 * @param size Size of returned list.
 * @return List of device information parameters which have the given
 * prefix.
 * */
const CL4DevQueryMap* cl4_devquery_list_prefix(
	gchar* prefix, gint* size) {
	
	/* Make sure prefix is not NULL. */
	g_return_val_if_fail(prefix != NULL, 0);
	
	/* Final prefix to search for. */
	gchar* prefix_final;
	
	/* Size of final prefix. */
	gint len_prefix_final;
	
	/* Search index. */
	gint idx_middle, idx_start, idx_end;
	
	/* Found info. */
	const CL4DevQueryMap* found_cl4_devquery_info_map = NULL;
	
	/* Determine final prefix according to how parameter names are
	 * stored in cl4_devquery_info_map. */
	prefix_final = cl4_devquery_get_prefix_final(prefix);
	
	/* Determine prefix size. */
	len_prefix_final = strlen(prefix_final);
	
	/* Binary search. */
	idx_middle = cl4_devquery_get_index(prefix_final);
	
	if (idx_middle >= 0) {
		/* Search for beginning. */
		idx_start = idx_middle;
		while (TRUE) {
			if (idx_start == 0)
				break;
			if (g_ascii_strncasecmp(prefix_final, 
				cl4_devquery_info_map[idx_start - 1].param_name, len_prefix_final) == 0)
				idx_start--;
			else
				break;
		}
		/* Search for ending. */
		idx_end = idx_middle;
		while (TRUE) {
			if (idx_end == cl4_devquery_info_map_size - 1)
				break;
			if (g_ascii_strncasecmp(prefix_final, 
				cl4_devquery_info_map[idx_end + 1].param_name, len_prefix_final) == 0)
				idx_end++;
			else
				break;
		}
		
		/* Set return values. */
		*size = idx_end - idx_start + 1;
		found_cl4_devquery_info_map = &cl4_devquery_info_map[idx_start];
		
	} else {
		
		/* Nothing found. */
		*size = -1;
		
	}

	/* Free final prefix. */
	g_free(prefix_final);
	
	/* Return result */
	return found_cl4_devquery_info_map;
}

/**
 * @brief Search for a device information parameter by matching part
 * of its name. This function is supposed to be used in a loop. 
 * 
 * @param substr String to match with parameter name.
 * @param idx Next index, should be zero in the first call, and the 
 * function updates within calls.
 * @return A matching CL4DevQueryMap, or NULL if search is over.
 * */
const CL4DevQueryMap* cl4_devquery_match(gchar* substr, gint* idx) {
	
	/* Make sure substr is not NULL. */
	g_return_val_if_fail(substr != NULL, NULL);
	
	/* Make sure idx is not NULL. */
	g_return_val_if_fail(idx != NULL, NULL);
	
	/* Found flag. */
	gboolean found = FALSE;

	/* Found result. */
	const CL4DevQueryMap* info_row = NULL;
	
	/* Linear search. */
	for ( ; *idx < cl4_devquery_info_map_size; (*idx)++) {
		if (g_strstr_len(
			cl4_devquery_info_map[*idx].param_name, -1, substr)) {
				
			found = TRUE;
			break;
		}
	}
	
	/* Set found result. */
	if (found)
		info_row = &cl4_devquery_info_map[*idx];

	/* Increment index (for next iteration). */
	(*idx)++;

	/* Return result. */
	return info_row;
	
}
