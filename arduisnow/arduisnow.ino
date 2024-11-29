#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "model.h"
#include "emul_sensors.h"

/* Globals, used for compatibility with Arduino-style sketches. */
namespace {
	tflite::ErrorReporter* error_reporter = nullptr;
	const tflite::Model *model = nullptr;
	tflite::MicroInterpreter *interpreter = nullptr;
	TfLiteTensor *input = nullptr;
	TfLiteTensor *output = nullptr;
	int inference_count = 0;

	constexpr int kTensorArenaSize = 2048;
	uint8_t tensor_arena[kTensorArenaSize];
	float iq_scale = 0.0;
	int32_t iq_zero = 0;
	
}  /* namespace */

inline int8_t quantize(float v, float scale, float zero_point) {
    int intv=static_cast<int>(v/scale + zero_point);
    
    /* return static_cast<int8_t> std::clamp(intv, INT8_MIN, INT8_MAX); */
    if (intv<INT8_MIN) return INT8_MIN;
    if (intv>INT8_MAX) return INT8_MAX;
    return static_cast<int8_t>(intv);
}

/* The name of this function is important for Arduino compatibility. */
void setup(void) 
{
	Serial.begin(115200);
	delay(2000);
	Serial.println("\n*** Starting");
	
	// Set up logging. Google style is to avoid globals or statics because of
	// lifetime uncertainty, but since this has a trivial destructor it's okay.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroErrorReporter micro_error_reporter;
	error_reporter = &micro_error_reporter;
	
	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	model = tflite::GetModel(g_snow_detector);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		TF_LITE_REPORT_ERROR(error_reporter,
		"Model provided is schema version %d not equal "
        "to supported version %d.",
		model->version(), TFLITE_SCHEMA_VERSION);
		return;
	}

	// This pulls in all the operation implementations we need.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::AllOpsResolver resolver;

	// Build an interpreter to run the model with.
	static tflite::MicroInterpreter static_interpreter(model, resolver, 
	       tensor_arena, kTensorArenaSize, error_reporter);
		   interpreter = &static_interpreter;
	
	// Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
		return;
	}
	
	// Obtain pointers to the model's input and output tensors.
	input = interpreter->input(0);
	output = interpreter->output(0);
	
	const auto* input_quantization =  reinterpret_cast<TfLiteAffineQuantization*>(input->quantization.params);
	iq_scale = input_quantization->scale->data[0];
	iq_zero  = input_quantization->zero_point->data[0];
}

constexpr float t_mean=10.24183450352789;
constexpr float t_std=8.14499814071525;
constexpr float h_mean=74.354998759613;
constexpr float h_std=17.225807868617938;

void loop(void)
{
	float t,h;
	
	int8_t input_tensor[6] = {0};
	
	while (1) {
		get_sensor_value(t, h);
		Serial.print("Temperature = ");
		Serial.print(t, 2);
		Serial.print("°C,    Humidity = ");
		Serial.print(h,2);
		Serial.println("%");
		
		// scale the data 
		t = (t-t_mean)/t_std;
		h = (h-h_mean)/h_std;
		
		// quantize
		input_tensor[0] = quantize(t,iq_scale,iq_zero);
		input_tensor[3] = quantize(h,iq_scale,iq_zero);
		if (inference_count>=2)
		{	  
			// Run inference, and report any error
			memcpy(input->data.int8, input_tensor, sizeof(int8_t)*6);
			TfLiteStatus invoke_status = interpreter->Invoke();
			if (invoke_status != kTfLiteOk) {
				 TF_LITE_REPORT_ERROR(error_reporter, "Error invoking TFLITE interpreter");
				 return;
			}
			int8 pred = output->data.int8[0];
			
			// we don't need to dequantize the output value
			// for our model, the sign of pred determines the class!
			Serial.print("pred = ");
			Serial.println(pred);
			if (pred>0) {
				Serial.println("Let it snows....");
			} else {
				Serial.println("Nope, no snow :(");
			}
		} else {
			Serial.println("Detector warming up");
		}
		
		// slide the data 
		input_tensor[2]=input_tensor[1];
		input_tensor[1]=input_tensor[0];
		input_tensor[5]=input_tensor[4];
		input_tensor[4]=input_tensor[3];
		
		inference_count++;
		delay(5000);
	}
}
