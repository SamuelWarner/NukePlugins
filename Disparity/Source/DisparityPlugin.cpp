// DisparityPlugin
// Author: Samuel Warner
// Version 0.5
// Copyright (C) 2018.
// 
// Plugin which takes a disparity map created from red and green color channels and applies
// pixel transformations based on those values to create a stereo image.


#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"

using namespace DD::Image;

static const char* const CLASS = "DisparityPlugin";
static const char* const HELP = "Mono to Stereo - offsets pixels in each eye by pixel values in disparity map"; 


class DisparityPlugin : public Iop {
	int leftView = 1;    //View number for left eye
	int rightView = 2;   //View number for right eye
	float _mrange[2];    //Range of multiply knob
	float multiplyValue; //Multiply value set in multiply knob

public:

	DisparityPlugin(Node* node) : Iop(node) {
		//set default values
		_mrange[0] = 0.0; 
		_mrange[1] = 5.0;
		multiplyValue = 1.0;
		
		inputs(2); //Set 2 inputs
	}

	const char* input_label(int, char*) const;
	virtual void knobs(Knob_Callback);
	int knob_changed(Knob*);

	void _validate(bool);
	void _request(int, int, int, int, ChannelMask, int); 
	void engine(int, int, int, ChannelMask, Row&); 

	const char* Class() const { return d.name; } 
	const char* node_help() const { return HELP; } 

	static const Iop::Description d; 
};

const char* DisparityPlugin::input_label(int input, char* buffer) const {
	switch (input) {
		case 0: //first input labeled image
			return "Img"; 
		case 1: //second input labeled disparity
			return "Dis";
		default:
			return 0;
	}
}

void DisparityPlugin::_validate(bool for_real) {
	copy_info(0); // Output copy input properties
}

void DisparityPlugin::_request(int x, int y, int r, int t, ChannelMask channels, int count) {
	// Request input data from both inputs
	input(0)->request(x, y, r, t, channels, count); 
	input(1)->request(x, y, r, t, channels, count);
}

void DisparityPlugin::engine(int yPos, int left, int right, ChannelMask channels, Row& row) {

	//Get image row from main image
	row.get(input0(), yPos, left, right, channels); 
	//Get image row from disparity map
	Row secondInputRow(left, right);
	secondInputRow.get(*input(1), yPos, left, right, channels);
	//create copy of image row to prevent overwritting during offset
	Row old(left, right);
	old.copy(row, channels, left, right);

	//Get pointers to disparity channel buffers
	const float* disparityRed = secondInputRow[Chan_Red]; 
	const float* disparityGreen = secondInputRow[Chan_Green]; 

	//loop through channels and apply disparity offset.
	foreach(chan, channels) {
		const float* inputBuffer = old[chan];
		float* outputBuffer = row.writable(chan);

		//offset for left eye
		if (outputContext().view() == leftView) { //process left view
			for (int x = left; x < right; x++) {
				int offset = int(disparityRed[x] * multiplyValue);
				int newPixel = x + offset;

				if (newPixel < right) {
					outputBuffer[newPixel] = inputBuffer[x];
				}
			}
		}

		//offset for right eye
		if (outputContext().view() == rightView) { //process right view
			for (int x = (right - 1); x >= left; x--) {
				int offset = int(disparityGreen[x] * multiplyValue);
				int newPixel = x - offset;

				if (newPixel >= left) {
					outputBuffer[newPixel] = inputBuffer[x];	
				}
			}
		}
	}

	

}

void DisparityPlugin::knobs(Knob_Callback f) {
	//create a multiply knob to amplify disparity effect
	Float_knob(f, &multiplyValue, NAME("multiply"), LABEL("Multiply")); 
	SetRange(f, _mrange[0], _mrange[1]);
	Tooltip(f, "Increase disparity map by this amount"); 

}

int DisparityPlugin::knob_changed(Knob* k) {
	//store changes to multiply knob for use in engine
	if (k->is("multiply")) {
		multiplyValue = k->get_value();
	}

	Iop::knob_changed(k);
	return 1;
}

static Iop* build(Node *node) {
	return new NukeWrapper(new DisparityPlugin(node));
}

const Iop::Description DisparityPlugin::d("DisparityPlugin", "DisparityPlugin", build); 