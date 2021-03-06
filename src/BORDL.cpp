#include "Bidoo.hpp"
#include "dsp/digital.hpp"
#include "BidooComponents.hpp"
#include <ctime>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

struct StepPlus {
	int index = 0;
	int number = 0;
	bool skip = false;
	bool skipParam = false;
	bool slide = false;
	int pulses = 1;
	int pulsesParam = 1;
	float pitch = 3.0f;
	int type = 2;
	float gateProb = 1.0f;
	float pitchRnd = 0.0f;
	float accent = 0.0f;
	float rndAccent = 0.0f;
};

struct PatternPlus {
	int playMode = 0;
	int countMode = 0;
	int numberOfSteps = 8;
	int numberOfStepsParam = 8;
	int rootNote = 0;
	int rootNoteParam = 0;
	int scale = 0;
	int scaleParam = 0;
	float gateTime = 0.5f;
	float slideTime = 0.2f;
	float sensitivity = 1.0f;
	int currentStep = 0;
	int currentPulse = 0;
	bool forward = true;
	std::vector<StepPlus> steps {16};

	void Update(int playMode, int countMode, int numberOfSteps, int numberOfStepsParam, int rootNote, int scale,
		 float gateTime, float slideTime, float sensitivity, std::vector<char> skips, std::vector<char> slides,
		  std::vector<Param> pulses, std::vector<Param> pitches, std::vector<Param> types, std::vector<Param> probGates,
			std::vector<Param> rndPitches, std::vector<Param> accents, std::vector<Param> rndAccents)
			{
		this->playMode = playMode;
		this->countMode = countMode;
		this->numberOfSteps = numberOfSteps;
		this->numberOfStepsParam = numberOfStepsParam;
		this->rootNote = rootNote;
		this->scale = scale;
		this->rootNoteParam = rootNote;
		this->scaleParam = scale;
		this->gateTime = gateTime;
		this->slideTime = slideTime;
		this->sensitivity = sensitivity;
		int pCount = 0;
		for (int i = 0; i < 16; i++) {
			steps[i].index = i%8;
			steps[i].number = i;
			if (((countMode == 0) && (i < numberOfSteps)) || ((countMode == 1) && (pCount < numberOfSteps))) {
				steps[i].skip = (skips[i%8] == 't');
			}	else {
				steps[i].skip = true;
			}
			steps[i].skipParam = (skips[i%8] == 't');
			steps[i].slide = (slides[i%8] == 't');
			if ((countMode == 1) && ((pCount + (int)pulses[i%8].value) >= numberOfSteps)) {
				steps[i].pulses = max(numberOfSteps - pCount, 0);
			}	else {
				steps[i].pulses = (int)pulses[i%8].value;
			}
			steps[i].pulsesParam = (int)pulses[i%8].value;
			pCount = pCount + steps[i].pulses;
			steps[i].pitch = pitches[i%8].value;
			steps[i].type = (int)types[i%8].value;
			steps[i].gateProb = probGates[i%8].value;
			steps[i].pitchRnd = rndPitches[i%8].value;
			steps[i].accent = accents[i%8].value;
			steps[i].rndAccent = rndAccents[i%8].value;
		}
	}

	std::tuple<int, int> GetNextStep(bool reset)
	{
		if (reset) {
			if (playMode != 1) {
				currentStep = GetFirstStep();
			} else {
				currentStep = GetLastStep();
			}
			currentPulse = 0;
			return std::make_tuple(currentStep,currentPulse);
		} else {
			if (currentPulse < steps[currentStep].pulses - 1) {
				currentPulse++;
				return std::make_tuple(steps[currentStep%16].index,currentPulse);
			} else {
				if (playMode == 0) {
					currentStep = GetNextStepForward(currentStep);
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 1) {
					currentStep = GetNextStepBackward(currentStep);
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 2) {
					if (currentStep == GetLastStep()) {
						forward = false;
					}
					if (currentStep == GetFirstStep()) {
						forward = true;
					}
					if (forward) {
						currentStep = GetNextStepForward(currentStep);
					} else {
						currentStep = GetNextStepBackward(currentStep);
					}
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 3) {
					std::vector<StepPlus> tmp (steps.size());
				  auto it = std::copy_if (steps.begin(), steps.end(), tmp.begin(), [](StepPlus i){return !(i.skip);} );
				  tmp.resize(std::distance(tmp.begin(),it));  // shrink container to new size
					StepPlus tmpStep = *select_randomly(tmp.begin(), tmp.end());
					currentPulse = 0;
					currentStep = tmpStep.number;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 4) {
					int next = GetNextStepForward(currentStep);
					int prev = GetNextStepBackward(currentStep);
					vector<StepPlus> subPattern;
					subPattern.push_back(steps[prev]);
					subPattern.push_back(steps[next]);
					StepPlus choice = *select_randomly(subPattern.begin(), subPattern.end());
					currentPulse = 0;
					currentStep = choice.number;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else {
					return std::make_tuple(0,0);
				}
			}
		}
	}

	StepPlus CurrentStep() {
		return this->steps[currentStep];
	}

	int GetFirstStep()
	{
			for (int i = 0; i < 16; i++) {
				if (!steps[i].skip) {
					return i;
				}
			}
			return 0;
	}

	int GetLastStep()
	{
			for (int i = 15; i >= 0  ; i--) {
				if (!steps[i].skip) {
					return i;
				}
			}
			return 15;
	}

	int GetNextStepForward(int pos)
	{
			for (int i = pos + 1; i < pos + 16; i++) {
				if (!steps[i%16].skip) {
					return i%16;
				}
			}
			return pos;
	}

	int GetNextStepBackward(int pos)
	{
			for (int i = pos - 1; i > pos - 16; i--) {
				if (!steps[i%16 + (i<0?16:0)].skip) {
					return i%16 + (i<0?16:0);
				}
			}
			return pos;
	}

	template<typename Iter, typename RandomGenerator> Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
		std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
		std::advance(start, dis(g));
		return start;
	}

	template<typename Iter> Iter select_randomly(Iter start, Iter end) {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		return select_randomly(start, end, gen);
	}

};

struct BORDL : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		SLIDE_TIME_PARAM,
		GATE_TIME_PARAM,
		ROOT_NOTE_PARAM,
		SCALE_PARAM,
		PLAY_MODE_PARAM,
		COUNT_MODE_PARAM,
		PATTERN_PARAM,
		SENSITIVITY_PARAM,
		TRIG_COUNT_PARAM = SENSITIVITY_PARAM + 8,
		TRIG_TYPE_PARAM = TRIG_COUNT_PARAM + 8,
		TRIG_PITCH_PARAM = TRIG_TYPE_PARAM + 8,
		TRIG_SLIDE_PARAM = TRIG_PITCH_PARAM + 8,
		TRIG_SKIP_PARAM = TRIG_SLIDE_PARAM + 8,
		TRIG_GATEPROB_PARAM = TRIG_SKIP_PARAM + 8,
		TRIG_PITCHRND_PARAM = TRIG_GATEPROB_PARAM + 8,
		TRIG_ACCENT_PARAM = TRIG_PITCHRND_PARAM + 8,
		TRIG_RNDACCENT_PARAM = TRIG_ACCENT_PARAM + 8,
		NUM_PARAMS = TRIG_RNDACCENT_PARAM + 8
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		SLIDE_TIME_INPUT,
		GATE_TIME_INPUT,
		ROOT_NOTE_INPUT,
		SCALE_INPUT,
		EXTGATE1_INPUT,
		EXTGATE2_INPUT,
		PATTERN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		PITCH_OUTPUT,
		ACC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATE_LIGHT,
		STEPS_LIGHTS = GATE_LIGHT + 8,
		SLIDES_LIGHTS = STEPS_LIGHTS + 8,
		SKIPS_LIGHTS = SLIDES_LIGHTS + 8,
		NUM_LIGHTS = SKIPS_LIGHTS + 8
	};

	//copied from http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
	int SCALE_AEOLIAN        [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_BLUES          [9] = {0, 2, 3, 4, 5, 7, 9, 10, 11};
	int SCALE_CHROMATIC      [12]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	int SCALE_DIATONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_DORIAN         [7] = {0, 2, 3, 5, 7, 9, 10};
	int SCALE_HARMONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 11};
	int SCALE_INDIAN         [7] = {0, 1, 1, 4, 5, 8, 10};
	int SCALE_LOCRIAN        [7] = {0, 1, 3, 5, 6, 8, 10};
	int SCALE_LYDIAN         [7] = {0, 2, 4, 6, 7, 9, 10};
	int SCALE_MAJOR          [7] = {0, 2, 4, 5, 7, 9, 11};
	int SCALE_MELODIC_MINOR  [9] = {0, 2, 3, 5, 7, 8, 9, 10, 11};
	int SCALE_MINOR          [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_MIXOLYDIAN     [7] = {0, 2, 4, 5, 7, 9, 10};
	int SCALE_NATURAL_MINOR  [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_PENTATONIC     [5] = {0, 2, 4, 7, 9};
	int SCALE_PHRYGIAN       [7] = {0, 1, 3, 5, 7, 8, 10};
	int SCALE_TURKISH        [7] = {0, 1, 3, 5, 7, 10, 11};

	enum Notes {
		NOTE_C,
		NOTE_C_SHARP,
		NOTE_D,
		NOTE_D_SHARP,
		NOTE_E,
		NOTE_F,
		NOTE_F_SHARP,
		NOTE_G,
		NOTE_G_SHARP,
		NOTE_A,
		NOTE_A_SHARP,
		NOTE_B,
		NUM_NOTES
	};

	enum Scales {
		AEOLIAN,
		BLUES,
		CHROMATIC,
		DIATONIC_MINOR,
		DORIAN,
		HARMONIC_MINOR,
		INDIAN,
		LOCRIAN,
		LYDIAN,
		MAJOR,
		MELODIC_MINOR,
		MINOR,
		MIXOLYDIAN,
		NATURAL_MINOR,
		PENTATONIC,
		PHRYGIAN,
		TURKISH,
		NONE,
		NUM_SCALES
	};

	bool running = true;
	SchmittTrigger clockTrigger;
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger slideTriggers[8];
	SchmittTrigger skipTriggers[8];
	SchmittTrigger playModeTrigger;
	SchmittTrigger countModeTrigger;
	SchmittTrigger PatternTrigger;
	float phase = 0.0f;
	int index = 0;
	int prevIndex = 0;
	bool reStart = true;
	int pulse = 0;
	int rootNote = 0;
	int curScaleVal = 0;
	float pitch = 0.0f;
	float previousPitch = 0.0f;
	clock_t tCurrent;
	clock_t tLastTrig;
	clock_t tPreviousTrig;
	std::vector<char> slideState = {'f','f','f','f','f','f','f','f'};
	std::vector<char> skipState = {'f','f','f','f','f','f','f','f'};
	int playMode = 0; // 0 forward, 1 backward, 2 pingpong, 3 random, 4 brownian
	int countMode = 0; // 0 steps, 1 pulses
	int numSteps = 8;
	int selectedPattern = 0;
	int playedPattern = 0;
	bool pitchMode = false;
	bool pitchQuantizeMode = true;
	bool updateFlag = false;
	bool probGate = true;
	float rndPitch = 0.0f;
	float accent = 5.0f;
	bool loadedFromJson = false;
	int copyPattern = -1;

	PatternPlus patterns[16];

	BORDL() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
	}

	void UpdatePattern() {
		std::vector<Param> pulses(&params[TRIG_COUNT_PARAM],&params[TRIG_COUNT_PARAM + 8]);
		std::vector<Param> pitches(&params[TRIG_PITCH_PARAM],&params[TRIG_PITCH_PARAM + 8]);
		std::vector<Param> types(&params[TRIG_TYPE_PARAM],&params[TRIG_TYPE_PARAM + 8]);
		std::vector<Param> probGates(&params[TRIG_GATEPROB_PARAM],&params[TRIG_GATEPROB_PARAM + 8]);
		std::vector<Param> rndPitches(&params[TRIG_PITCHRND_PARAM],&params[TRIG_PITCHRND_PARAM + 8]);
		std::vector<Param> accents(&params[TRIG_ACCENT_PARAM],&params[TRIG_ACCENT_PARAM + 8]);
		std::vector<Param> rndAccents(&params[TRIG_RNDACCENT_PARAM],&params[TRIG_RNDACCENT_PARAM + 8]);
		patterns[selectedPattern].Update(playMode, countMode, numSteps, roundf(params[STEPS_PARAM].value), roundf(params[ROOT_NOTE_PARAM].value),
		 roundf(params[SCALE_PARAM].value), params[GATE_TIME_PARAM].value, params[SLIDE_TIME_PARAM].value, params[SENSITIVITY_PARAM].value ,
		  skipState, slideState, pulses, pitches, types, probGates, rndPitches, accents, rndAccents);
	}

	void step() override;

	// persistence, random & init

	json_t *toJson() override {
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "running", json_boolean(running));
		json_object_set_new(rootJ, "playMode", json_integer(playMode));
		json_object_set_new(rootJ, "countMode", json_integer(countMode));
		json_object_set_new(rootJ, "pitchMode", json_boolean(pitchMode));
		json_object_set_new(rootJ, "pitchQuantizeMode", json_boolean(pitchQuantizeMode));
		json_object_set_new(rootJ, "selectedPattern", json_integer(selectedPattern));
		json_object_set_new(rootJ, "playedPattern", json_integer(playedPattern));

		json_t *trigsJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t *trigJ = json_array();
			json_t *trigJSlide = json_boolean(slideState[i] == 't');
			json_array_append_new(trigJ, trigJSlide);
			json_t *trigJSkip = json_boolean(skipState[i] == 't');
			json_array_append_new(trigJ, trigJSkip);
			json_array_append_new(trigsJ, trigJ);
		}
		json_object_set_new(rootJ, "trigs", trigsJ);

		for (int i = 0; i<16; i++) {
			json_t *patternJ = json_object();
			json_object_set_new(patternJ, "playMode", json_integer(patterns[i].playMode));
			json_object_set_new(patternJ, "countMode", json_integer(patterns[i].countMode));
			json_object_set_new(patternJ, "numSteps", json_integer(patterns[i].numberOfStepsParam));
			json_object_set_new(patternJ, "rootNote", json_integer(patterns[i].rootNote));
			json_object_set_new(patternJ, "scale", json_integer(patterns[i].scale));
			json_object_set_new(patternJ, "gateTime", json_real(patterns[i].gateTime));
			json_object_set_new(patternJ, "slideTime", json_real(patterns[i].slideTime));
			json_object_set_new(patternJ, "sensitivity", json_real(patterns[i].sensitivity));
			for (int j = 0; j < 16; j++) {
				json_t *stepJ = json_object();
				json_object_set_new(stepJ, "index", json_integer(patterns[i].steps[j].index));
				json_object_set_new(stepJ, "number", json_integer(patterns[i].steps[j].number));
				json_object_set_new(stepJ, "skip", json_integer((int)patterns[i].steps[j].skip));
				json_object_set_new(stepJ, "skipParam", json_integer((int)patterns[i].steps[j].skipParam));
				json_object_set_new(stepJ, "slide", json_integer((int)patterns[i].steps[j].slide));
				json_object_set_new(stepJ, "pulses", json_integer(patterns[i].steps[j].pulses));
				json_object_set_new(stepJ, "pulsesParam", json_integer(patterns[i].steps[j].pulsesParam));
				json_object_set_new(stepJ, "pitch", json_real(patterns[i].steps[j].pitch));
				json_object_set_new(stepJ, "type", json_integer(patterns[i].steps[j].type));
				json_object_set_new(stepJ, "gateProb", json_real(patterns[i].steps[j].gateProb));
				json_object_set_new(stepJ, "pitchRnd", json_real(patterns[i].steps[j].pitchRnd));
				json_object_set_new(stepJ, "accent", json_real(patterns[i].steps[j].accent));
				json_object_set_new(stepJ, "rndAccent", json_real(patterns[i].steps[j].rndAccent));
				json_object_set_new(patternJ, ("step" + to_string(j)).c_str() , stepJ);
			}
			json_object_set_new(rootJ, ("pattern" + to_string(i)).c_str(), patternJ);
		}
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);
		json_t *playModeJ = json_object_get(rootJ, "playMode");
		if (playModeJ)
			playMode = json_integer_value(playModeJ);
		json_t *countModeJ = json_object_get(rootJ, "countMode");
		if (countModeJ)
			countMode = json_integer_value(countModeJ);
		json_t *selectedPatternJ = json_object_get(rootJ, "selectedPattern");
		if (selectedPatternJ)
			selectedPattern = json_integer_value(selectedPatternJ);
		json_t *playedPatternJ = json_object_get(rootJ, "playedPattern");
		if (playedPatternJ)
			playedPattern = json_integer_value(playedPatternJ);
		json_t *pitchModeJ = json_object_get(rootJ, "pitchMode");
		if (pitchModeJ)
			pitchMode = json_is_true(pitchModeJ);
		json_t *pitchQuantizeModeJ = json_object_get(rootJ, "pitchQuantizeMode");
		if (pitchQuantizeModeJ)
			pitchQuantizeMode = json_is_true(pitchQuantizeModeJ);
		json_t *trigsJ = json_object_get(rootJ, "trigs");
		if (trigsJ) {
			for (int i = 0; i < 8; i++) {
				json_t *trigJ = json_array_get(trigsJ, i);
				if (trigJ)
				{
					slideState[i] = json_is_true(json_array_get(trigJ, 0)) ? 't' : 'f';
					skipState[i] = json_is_true(json_array_get(trigJ, 1)) ? 't' : 'f';
				}
			}
		}
		for (int i=0; i<16;i++) {
			json_t *patternJ = json_object_get(rootJ, ("pattern" + to_string(i)).c_str());
			if (patternJ){
				json_t *playModeJ = json_object_get(patternJ, "playMode");
				if (playModeJ)
					patterns[i].playMode = json_integer_value(playModeJ);
				json_t *countModeJ = json_object_get(patternJ, "countMode");
				if (countModeJ)
					patterns[i].countMode = json_integer_value(countModeJ);
				json_t *numStepsJ = json_object_get(patternJ, "numSteps");
				if (numStepsJ)
					patterns[i].numberOfStepsParam = json_integer_value(numStepsJ);
				json_t *rootNoteJ = json_object_get(patternJ, "rootNote");
				if (rootNoteJ)
					patterns[i].rootNote = json_integer_value(rootNoteJ);
				json_t *scaleJ = json_object_get(patternJ, "scale");
				if (scaleJ)
					patterns[i].scale = json_integer_value(scaleJ);
				json_t *gateTimeJ = json_object_get(patternJ, "gateTime");
				if (gateTimeJ)
					patterns[i].gateTime = json_real_value(gateTimeJ);
				json_t *slideTimeJ = json_object_get(patternJ, "slideTime");
				if (slideTimeJ)
					patterns[i].slideTime = json_real_value(slideTimeJ);
				json_t *sensitivityJ = json_object_get(patternJ, "sensitivity");
				if (sensitivityJ)
					patterns[i].sensitivity = json_real_value(sensitivityJ);
				for (int j = 0; j < 16; j++) {
					json_t *stepJ = json_object_get(patternJ, ("step" + to_string(j)).c_str());
					if (stepJ) {
						json_t *indexJ= json_object_get(stepJ, "index");
						if (indexJ)
							patterns[i].steps[j].index = json_integer_value(indexJ);
						json_t *numberJ= json_object_get(stepJ, "numer");
						if (numberJ)
							patterns[i].steps[j].number = json_integer_value(numberJ);
						json_t *skipJ= json_object_get(stepJ, "skip");
						if (skipJ)
							patterns[i].steps[j].skip = !!json_integer_value(skipJ);
						json_t *skipParamJ= json_object_get(stepJ, "skipParam");
						if (skipParamJ)
							patterns[i].steps[j].skipParam = !!json_integer_value(skipParamJ);
						json_t *slideJ= json_object_get(stepJ, "slide");
						if (slideJ)
							patterns[i].steps[j].slide = !!json_integer_value(slideJ);
						json_t *pulsesJ= json_object_get(stepJ, "pulses");
						if (pulsesJ)
							patterns[i].steps[j].pulses = json_integer_value(pulsesJ);
						json_t *pulsesParamJ= json_object_get(stepJ, "pulsesParam");
						if (pulsesParamJ)
							patterns[i].steps[j].pulsesParam = json_integer_value(pulsesParamJ);
						json_t *pitchJ= json_object_get(stepJ, "pitch");
						if (pitchJ)
							patterns[i].steps[j].pitch = json_real_value(pitchJ);
						json_t *typeJ= json_object_get(stepJ, "type");
						if (typeJ)
							patterns[i].steps[j].type = json_integer_value(typeJ);
						json_t *probGateJ= json_object_get(stepJ, "gateProb");
						if (probGateJ)
							patterns[i].steps[j].gateProb = json_real_value(probGateJ);
						json_t *rndPitchJ= json_object_get(stepJ, "pitchRnd");
						if (rndPitchJ)
							patterns[i].steps[j].pitchRnd = json_real_value(rndPitchJ);
						json_t *accentJ= json_object_get(stepJ, "accent");
						if (accentJ)
							patterns[i].steps[j].accent = json_real_value(accentJ);
						json_t *rndAccentJ= json_object_get(stepJ, "rndAccent");
						if (rndAccentJ)
							patterns[i].steps[j].rndAccent = json_real_value(rndAccentJ);
					}
				}
			}
		}
		updateFlag = true;
		loadedFromJson = true;
	}

	void randomize() override {
		randomizeSlidesSkips();
	}

	void randomizeSlidesSkips() {
		for (int i = 0; i < 8; i++) {
			slideState[i] = (randomUniform() > 0.8f) ? 't' : 'f';
			skipState[i] = (randomUniform() > 0.85f) ? 't' : 'f';
		}
	}

	void reset() override {
		for (int i = 0; i < 8; i++) {
			slideState[i] = 'f';
			skipState[i] = 'f';
		}
	}

	// Quantization inspired from  https://github.com/jeremywen/JW-Modules

	float getOneRandomNoteInScale(){
		rootNote = clamp(patterns[playedPattern].rootNote + inputs[ROOT_NOTE_INPUT].value, 0.0f, NUM_NOTES-1.0f);
		curScaleVal = clamp(patterns[playedPattern].scale + inputs[SCALE_INPUT].value, 0.0f, NUM_SCALES-1.0f);
		int *curScaleArr;
		int notesInScale = 0;
		switch(curScaleVal){
			case AEOLIAN:        curScaleArr = SCALE_AEOLIAN;       notesInScale=LENGTHOF(SCALE_AEOLIAN); break;
			case BLUES:          curScaleArr = SCALE_BLUES;         notesInScale=LENGTHOF(SCALE_BLUES); break;
			case CHROMATIC:      curScaleArr = SCALE_CHROMATIC;     notesInScale=LENGTHOF(SCALE_CHROMATIC); break;
			case DIATONIC_MINOR: curScaleArr = SCALE_DIATONIC_MINOR;notesInScale=LENGTHOF(SCALE_DIATONIC_MINOR); break;
			case DORIAN:         curScaleArr = SCALE_DORIAN;        notesInScale=LENGTHOF(SCALE_DORIAN); break;
			case HARMONIC_MINOR: curScaleArr = SCALE_HARMONIC_MINOR;notesInScale=LENGTHOF(SCALE_HARMONIC_MINOR); break;
			case INDIAN:         curScaleArr = SCALE_INDIAN;        notesInScale=LENGTHOF(SCALE_INDIAN); break;
			case LOCRIAN:        curScaleArr = SCALE_LOCRIAN;       notesInScale=LENGTHOF(SCALE_LOCRIAN); break;
			case LYDIAN:         curScaleArr = SCALE_LYDIAN;        notesInScale=LENGTHOF(SCALE_LYDIAN); break;
			case MAJOR:          curScaleArr = SCALE_MAJOR;         notesInScale=LENGTHOF(SCALE_MAJOR); break;
			case MELODIC_MINOR:  curScaleArr = SCALE_MELODIC_MINOR; notesInScale=LENGTHOF(SCALE_MELODIC_MINOR); break;
			case MINOR:          curScaleArr = SCALE_MINOR;         notesInScale=LENGTHOF(SCALE_MINOR); break;
			case MIXOLYDIAN:     curScaleArr = SCALE_MIXOLYDIAN;    notesInScale=LENGTHOF(SCALE_MIXOLYDIAN); break;
			case NATURAL_MINOR:  curScaleArr = SCALE_NATURAL_MINOR; notesInScale=LENGTHOF(SCALE_NATURAL_MINOR); break;
			case PENTATONIC:     curScaleArr = SCALE_PENTATONIC;    notesInScale=LENGTHOF(SCALE_PENTATONIC); break;
			case PHRYGIAN:       curScaleArr = SCALE_PHRYGIAN;      notesInScale=LENGTHOF(SCALE_PHRYGIAN); break;
			case TURKISH:        curScaleArr = SCALE_TURKISH;       notesInScale=LENGTHOF(SCALE_TURKISH); break;
		}

		if(curScaleVal == NONE){
			return randomUniform() * 6.0f;
		} else {
			float voltsOut = 0.0f;
			int rndOctaveInVolts = int(5 * randomUniform());
			voltsOut += rndOctaveInVolts;
			voltsOut += rootNote / 12.0f;
			voltsOut += curScaleArr[int(notesInScale * randomUniform())] / 12.0f;
			return voltsOut;
		}
	}

	float closestVoltageInScale(float voltsIn){
		rootNote = clamp(patterns[playedPattern].rootNote + inputs[ROOT_NOTE_INPUT].value, 0.0f, BORDL::NUM_NOTES-1.0f);
		curScaleVal = clamp(patterns[playedPattern].scale + inputs[SCALE_INPUT].value, 0.0f, BORDL::NUM_SCALES-1.0f);
		int *curScaleArr;
		int notesInScale = 0;
		switch(curScaleVal){
			case AEOLIAN:        curScaleArr = SCALE_AEOLIAN;       notesInScale=LENGTHOF(SCALE_AEOLIAN); break;
			case BLUES:          curScaleArr = SCALE_BLUES;         notesInScale=LENGTHOF(SCALE_BLUES); break;
			case CHROMATIC:      curScaleArr = SCALE_CHROMATIC;     notesInScale=LENGTHOF(SCALE_CHROMATIC); break;
			case DIATONIC_MINOR: curScaleArr = SCALE_DIATONIC_MINOR;notesInScale=LENGTHOF(SCALE_DIATONIC_MINOR); break;
			case DORIAN:         curScaleArr = SCALE_DORIAN;        notesInScale=LENGTHOF(SCALE_DORIAN); break;
			case HARMONIC_MINOR: curScaleArr = SCALE_HARMONIC_MINOR;notesInScale=LENGTHOF(SCALE_HARMONIC_MINOR); break;
			case INDIAN:         curScaleArr = SCALE_INDIAN;        notesInScale=LENGTHOF(SCALE_INDIAN); break;
			case LOCRIAN:        curScaleArr = SCALE_LOCRIAN;       notesInScale=LENGTHOF(SCALE_LOCRIAN); break;
			case LYDIAN:         curScaleArr = SCALE_LYDIAN;        notesInScale=LENGTHOF(SCALE_LYDIAN); break;
			case MAJOR:          curScaleArr = SCALE_MAJOR;         notesInScale=LENGTHOF(SCALE_MAJOR); break;
			case MELODIC_MINOR:  curScaleArr = SCALE_MELODIC_MINOR; notesInScale=LENGTHOF(SCALE_MELODIC_MINOR); break;
			case MINOR:          curScaleArr = SCALE_MINOR;         notesInScale=LENGTHOF(SCALE_MINOR); break;
			case MIXOLYDIAN:     curScaleArr = SCALE_MIXOLYDIAN;    notesInScale=LENGTHOF(SCALE_MIXOLYDIAN); break;
			case NATURAL_MINOR:  curScaleArr = SCALE_NATURAL_MINOR; notesInScale=LENGTHOF(SCALE_NATURAL_MINOR); break;
			case PENTATONIC:     curScaleArr = SCALE_PENTATONIC;    notesInScale=LENGTHOF(SCALE_PENTATONIC); break;
			case PHRYGIAN:       curScaleArr = SCALE_PHRYGIAN;      notesInScale=LENGTHOF(SCALE_PHRYGIAN); break;
			case TURKISH:        curScaleArr = SCALE_TURKISH;       notesInScale=LENGTHOF(SCALE_TURKISH); break;
			case NONE:           return voltsIn;
		}

		float closestVal = 10.0f;
		float closestDist = 10.0f;
		int octaveInVolts = int(voltsIn);
		for (int i = 0; i < notesInScale; i++) {
			float scaleNoteInVolts = octaveInVolts + (((pitchQuantizeMode ? rootNote : 0.0f) + curScaleArr[i]) / 12.0f);
			float distAway = fabs(voltsIn - scaleNoteInVolts);
			if(distAway < closestDist){
				closestVal = scaleNoteInVolts;
				closestDist = distAway;
			}
		}
		closestVal += pitchQuantizeMode ? 0.0f : (rootNote / 12.0f);
		return closestVal;
	}
};


void BORDL::step() {
 	const float lightLambda = 0.075f;
	// Run
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
	}
	lights[RUNNING_LIGHT].value = running ? 1.0f : 0.0f;
	bool nextStep = false;
	// Phase calculation
	if (running) {
		if (inputs[EXT_CLOCK_INPUT].active) {
			tCurrent = clock();
			float clockTime = powf(2.0f, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			if (tLastTrig && tPreviousTrig) {
				phase = float(tCurrent - tLastTrig) / float(tLastTrig - tPreviousTrig);
			}
			else {
				phase += clockTime / engineGetSampleRate();
			}
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
				tPreviousTrig = tLastTrig;
				tLastTrig = clock();
				phase = 0.0f;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0f, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime / engineGetSampleRate();
			if (phase >= 1.0f) {
				phase--;
				nextStep = true;
			}
		}
	}
	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
		phase = 0.0f;
		reStart = true;
		nextStep = true;
		lights[RESET_LIGHT].value = 1.0f;
	}
	//patternNumber
	playedPattern = clamp((inputs[PATTERN_INPUT].active ? rescale(inputs[PATTERN_INPUT].value,0.0f,10.0f,1.0f,16.1f) : params[PATTERN_PARAM].value) - 1.0f, 0.0f, 15.0f);
	// Update Pattern
	if ((updateFlag) || (!loadedFromJson)) {
		// Trigs Update
		for (int i = 0; i < 8; i++) {
			if (slideTriggers[i].process(params[TRIG_SLIDE_PARAM + i].value)) {
				slideState[i] = slideState[i] == 't' ? 'f' : 't';
			}
			if (skipTriggers[i].process(params[TRIG_SKIP_PARAM + i].value)) {
				skipState[i] = skipState[i] == 't' ? 'f' : 't';
			}
		}
		// playMode
		if (playModeTrigger.process(params[PLAY_MODE_PARAM].value)) {
			playMode = (((int)playMode + 1) % 5);
		}
		// countMode
		if (countModeTrigger.process(params[COUNT_MODE_PARAM].value)) {
			countMode = (((int)countMode + 1) % 2);
		}
		// numSteps
		numSteps = clamp(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1.0f, 16.0f);
		UpdatePattern();
		if (!loadedFromJson) {
			loadedFromJson = true;
			updateFlag = true;
		}
	}
	// Steps && Pulses Management
	if (nextStep) {
		// Advance step
		previousPitch = closestVoltageInScale(patterns[playedPattern].CurrentStep().pitch);
		prevIndex = index;
		auto nextT = patterns[playedPattern].GetNextStep(reStart);
		index = std::get<0>(nextT);
		pulse = std::get<1>(nextT);
		if (reStart) { reStart = false; }
		lights[STEPS_LIGHTS+index%8].value = 1.0f;
		probGate = index != prevIndex ? randomUniform() <= params[TRIG_GATEPROB_PARAM+index%8].value : probGate;
		rndPitch = index != prevIndex ? rescale(randomUniform(),0.0f,1.0f,params[TRIG_PITCHRND_PARAM+index%8].value*-5.0f,params[TRIG_PITCHRND_PARAM+index%8].value*5.0f) : rndPitch;
		accent = index != prevIndex ? clamp(params[TRIG_ACCENT_PARAM+index%8].value + rescale(randomUniform(),0.0f,1.0f,params[TRIG_RNDACCENT_PARAM+index%8].value*-5.0f,params[TRIG_RNDACCENT_PARAM+index%8].value*5.0f),0.0f,10.0f)  : accent;
	}
	// Lights
	for (int i = 0; i < 8; i++) {
		lights[STEPS_LIGHTS + i].value -= lights[STEPS_LIGHTS + i].value / lightLambda / engineGetSampleRate();
		lights[SLIDES_LIGHTS + i].value = slideState[i] == 't' ? 1.0f - lights[STEPS_LIGHTS + i].value : lights[STEPS_LIGHTS + i].value;
		lights[SKIPS_LIGHTS + i].value = skipState[i]== 't' ? 1.0f - lights[STEPS_LIGHTS + i].value : lights[STEPS_LIGHTS + i].value;
	}
	lights[RESET_LIGHT].value -= lights[RESET_LIGHT].value / lightLambda / engineGetSampleRate();

	// Caclulate Outputs
	bool gateOn = running && (!patterns[playedPattern].CurrentStep().skip);
	float gateValue = 0.0f;
	if (gateOn){
		if (patterns[playedPattern].CurrentStep().type == 0) {
			gateOn = false;
		}
		else if (((patterns[playedPattern].CurrentStep().type == 1) && (pulse == 0))
				|| (patterns[playedPattern].CurrentStep().type == 2)
				|| ((patterns[playedPattern].CurrentStep().type == 3) && (pulse == patterns[playedPattern].CurrentStep().pulses))) {
				float gateCoeff = clamp(patterns[playedPattern].gateTime - 0.02f + inputs[GATE_TIME_INPUT].value /10.0f, 0.0f, 0.99f);
			gateOn = phase < gateCoeff;
			gateValue = 10.0f;
		}
		else if (patterns[playedPattern].CurrentStep().type == 3) {
			gateOn = true;
			gateValue = 10.0f;
		}
		else if (patterns[playedPattern].CurrentStep().type == 4) {
			gateOn = true;
			gateValue = inputs[EXTGATE1_INPUT].value;
		}
		else if (patterns[playedPattern].CurrentStep().type == 5) {
			gateOn = true;
			gateValue = inputs[EXTGATE2_INPUT].value;
		}
		else {
			gateOn = false;
			gateValue = 0.0f;
		}
	}
	//pitch management
	pitch = closestVoltageInScale(clamp(patterns[playedPattern].CurrentStep().pitch + rndPitch,0.0f,10.0f) * patterns[playedPattern].sensitivity);
	if (patterns[playedPattern].CurrentStep().slide) {
		if (pulse == 0) {
			float slideCoeff = clamp(patterns[playedPattern].slideTime - 0.01f + inputs[SLIDE_TIME_INPUT].value /10.0f, -0.1f, 0.99f);
			pitch = pitch - (1.0f - powf(phase, slideCoeff)) * (pitch - previousPitch);
		}
	}

	// Update Outputs
	outputs[GATE_OUTPUT].value = gateOn ? (probGate ? gateValue : 0.0f) : 0.0f;
	outputs[PITCH_OUTPUT].value = pitchMode ? pitch : (gateOn ? pitch : 0.0f);
	outputs[ACC_OUTPUT].value = gateOn ? (probGate ? accent : 0.0f) : 0.0f;
}

struct BORDLDisplay : TransparentWidget {
	BORDL *module;
	int frame = 0;
	shared_ptr<Font> font;

	string note, scale, steps, playMode, selectedPattern, playedPattern;

	BORDLDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawMessage(NVGcontext *vg, Vec pos, string note, string playMode, string selectedPattern, string playedPattern, string steps, string scale) {
		nvgFontSize(vg, 18.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgText(vg, pos.x + 4.0f, pos.y + 8.0f, playMode.c_str(), NULL);
		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgText(vg, pos.x + 91.0f, pos.y + 7.0f, selectedPattern.c_str(), NULL);
		nvgText(vg, pos.x + 31.0f, pos.y + 7.0f, steps.c_str(), NULL);
		nvgText(vg, pos.x + 3.0f, pos.y + 23.0f, note.c_str(), NULL);
		nvgText(vg, pos.x + 25.0f, pos.y + 23.0f, scale.c_str(), NULL);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgText(vg, pos.x + 116.0f, pos.y + 7.0f, playedPattern.c_str(), NULL);
	}

	string displayRootNote(int value) {
		switch(value){
			case BORDL::NOTE_C:       return "C";
			case BORDL::NOTE_C_SHARP: return "C#";
			case BORDL::NOTE_D:       return "D";
			case BORDL::NOTE_D_SHARP: return "D#";
			case BORDL::NOTE_E:       return "E";
			case BORDL::NOTE_F:       return "F";
			case BORDL::NOTE_F_SHARP: return "F#";
			case BORDL::NOTE_G:       return "G";
			case BORDL::NOTE_G_SHARP: return "G#";
			case BORDL::NOTE_A:       return "A";
			case BORDL::NOTE_A_SHARP: return "A#";
			case BORDL::NOTE_B:       return "B";
			default: return "";
		}
	}

	string displayScale(int value) {
		switch(value){
			case BORDL::AEOLIAN:        return "Aeolian";
			case BORDL::BLUES:          return "Blues";
			case BORDL::CHROMATIC:      return "Chromatic";
			case BORDL::DIATONIC_MINOR: return "Diatonic Minor";
			case BORDL::DORIAN:         return "Dorian";
			case BORDL::HARMONIC_MINOR: return "Harmonic Minor";
			case BORDL::INDIAN:         return "Indian";
			case BORDL::LOCRIAN:        return "Locrian";
			case BORDL::LYDIAN:         return "Lydian";
			case BORDL::MAJOR:          return "Major";
			case BORDL::MELODIC_MINOR:  return "Melodic Minor";
			case BORDL::MINOR:          return "Minor";
			case BORDL::MIXOLYDIAN:     return "Mixolydian";
			case BORDL::NATURAL_MINOR:  return "Natural Minor";
			case BORDL::PENTATONIC:     return "Pentatonic";
			case BORDL::PHRYGIAN:       return "Phrygian";
			case BORDL::TURKISH:        return "Turkish";
			case BORDL::NONE:           return "None";
			default: return "";
		}
	}

	string displayPlayMode(int value) {
		switch(value){
			case 0: return "►";
			case 1: return "◄";
			case 2: return "►◄";
			case 3: return "►*";
			case 4: return "►?";
			default: return "";
		}
	}

	void draw(NVGcontext *vg) override {
		if (++frame >= 4) {
			frame = 0;
			note = displayRootNote(module->patterns[module->selectedPattern].rootNote);
			steps = (module->patterns[module->selectedPattern].countMode == 0 ? "steps:" : "pulses:" ) + to_string(module->patterns[module->selectedPattern].numberOfStepsParam);
			playMode = displayPlayMode(module->patterns[module->selectedPattern].playMode);
			scale = displayScale(module->patterns[module->selectedPattern].scale);
			selectedPattern = "P" + to_string(module->selectedPattern + 1);
			playedPattern = "P" + to_string(module->playedPattern + 1);
		}
		drawMessage(vg, Vec(0.0f, 20.0f), note, playMode, selectedPattern, playedPattern, steps, scale);
	}
};

struct BORDLGateDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLGateDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawGate(NVGcontext *vg, Vec pos) {
		int gateType = (int)module->params[BORDL::TRIG_TYPE_PARAM+index].value;
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		if (gateType == 0) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
		}
		else if (gateType == 1) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,6.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 2) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,6.0f,6.0f,0.0f);
			nvgRoundedRect(vg,pos.x+8.0f,pos.y,6.0f,6.0f,0.0f);
			nvgRoundedRect(vg,pos.x+16.0f,pos.y,6.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 3) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 4) {
		  nvgText(vg, pos.x+11.0f, pos.y+8.0f, "G1", NULL);
		}
		else if (gateType == 5) {
		  nvgText(vg, pos.x+11.0f, pos.y+8.0f, "G2", NULL);
		}
	}

	void draw(NVGcontext *vg) override {
		drawGate(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLPulseDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLPulseDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawPulse(NVGcontext *vg, Vec pos) {
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		char tCount[128],tProb[128];
		snprintf(tCount, sizeof(tCount), "%1i", (int)module->params[BORDL::TRIG_COUNT_PARAM+index].value);
		snprintf(tProb, sizeof(tProb), "%1i%%", (int)(module->params[BORDL::TRIG_GATEPROB_PARAM+index].value*100.0f));
		nvgText(vg, pos.x, pos.y, tCount, NULL);
		nvgFontSize(vg, 12.0f);
		nvgText(vg, pos.x, pos.y+12.0f, tProb, NULL);
	}

	void draw(NVGcontext *vg) override {
		drawPulse(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLPitchDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLPitchDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawPitch(NVGcontext *vg, Vec pos) {
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 14.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		char tPitch[128],tPitchRnd[128],tAcc[128],tAccRnd[128];
		snprintf(tPitch, sizeof(tPitch), "%1.2f", module->closestVoltageInScale(module->params[BORDL::TRIG_PITCH_PARAM+index].value));
		snprintf(tPitchRnd, sizeof(tPitchRnd), "%1i%%", (int)(module->params[BORDL::TRIG_PITCHRND_PARAM+index].value*100));
		snprintf(tAcc, sizeof(tAcc), "%1.2f", module->params[BORDL::TRIG_ACCENT_PARAM+index].value);
		snprintf(tAccRnd, sizeof(tAccRnd), "%1i%%", (int)(module->params[BORDL::TRIG_RNDACCENT_PARAM+index].value*100));
		nvgText(vg, pos.x, pos.y-9.0f, tPitch, NULL);
		nvgText(vg, pos.x, pos.y+15.0f, tAcc, NULL);
		nvgFontSize(vg, 12.0f);
		nvgText(vg, pos.x, pos.y+2.0f, tPitchRnd, NULL);
		nvgText(vg, pos.x, pos.y+25.0f, tAccRnd, NULL);
	}

	void draw(NVGcontext *vg) override {
		drawPitch(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLWidget : ModuleWidget {
	ParamWidget *stepsParam, *scaleParam, *rootNoteParam, *sensitivityParam,
	 *gateTimeParam, *slideTimeParam, *playModeParam, *countModeParam, *patternParam,
		*pitchParams[8], *pulseParams[8], *typeParams[8], *slideParams[8], *skipParams[8],
		 *pitchRndParams[8], *pulseProbParams[8], *accentParams[8], *rndAccentParams[8];

	BORDLWidget(BORDL *module);
	Menu *createContextMenu() override;
};

struct BORDLPatternRoundBlackSnapKnob : RoundBlackSnapKnob {
	void onChange(EventChange &e) override {
			RoundBlackSnapKnob::onChange(e);
			BORDL *module = dynamic_cast<BORDL*>(this->module);
			BORDLWidget *parent = dynamic_cast<BORDLWidget*>(this->parent);
			int target = clamp(value - 1.0f, 0.0f, 15.0f);
			if (module && parent && (target != module->selectedPattern) && module->updateFlag)
			{
				module->updateFlag = false;
				module->selectedPattern = value - 1;
				parent->stepsParam->setValue(module->patterns[target].numberOfStepsParam);
				parent->rootNoteParam->setValue(module->patterns[target].rootNote);
				parent->scaleParam->setValue(module->patterns[target].scale);
				parent->gateTimeParam->setValue(module->patterns[target].gateTime);
				parent->slideTimeParam->setValue(module->patterns[target].slideTime);
				parent->sensitivityParam->setValue(module->patterns[target].sensitivity);
				module->playMode = module->patterns[module->selectedPattern].playMode;
				module->countMode = module->patterns[module->selectedPattern].countMode;
				for (int i = 0; i < 8; i++) {
					parent->pitchParams[i]->setValue(module->patterns[target].steps[i].pitch);
					parent->pulseParams[i]->setValue(module->patterns[target].steps[i].pulsesParam);
					parent->typeParams[i]->setValue(module->patterns[target].steps[i].type);
					module->skipState[i] = module->patterns[target].steps[i].skipParam ? 't' : 'f';
					module->slideState[i] = module->patterns[target].steps[i].slide ? 't' : 'f';
					parent->pulseProbParams[i]->setValue(module->patterns[target].steps[i].gateProb);
					parent->pitchRndParams[i]->setValue(module->patterns[target].steps[i].pitchRnd);
					parent->accentParams[i]->setValue(module->patterns[target].steps[i].accent);
					parent->rndAccentParams[i]->setValue(module->patterns[target].steps[i].rndAccent);
				}
				module->updateFlag = true;
			}
		}
};



BORDLWidget::BORDLWidget(BORDL *module) : ModuleWidget(module) {
	setPanel(SVG::load(assetPlugin(plugin, "res/BORDL.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	{
		BORDLDisplay *display = new BORDLDisplay();
		display->module = module;
		display->box.pos = Vec(20.0f, 253.0f);
		display->box.size = Vec(250.0f, 60.0f);
		addChild(display);
	}

	addParam(ParamWidget::create<RoundBlackKnob>(Vec(17.0f, 52.0f), module, BORDL::CLOCK_PARAM, -2.0f, 6.0f, 2.0f));
	addParam(ParamWidget::create<LEDButton>(Vec(61.0f, 56.0f), module, BORDL::RUN_PARAM, 0.0f, 1.0f, 0.0f));
	addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(67.0f, 62.0f), module, BORDL::RUNNING_LIGHT));
	addParam(ParamWidget::create<LEDButton>(Vec(99.0f, 56.0f), module, BORDL::RESET_PARAM, 0.0f, 1.0f, 0.0f));
	addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(105.0f, 62.0f), module, BORDL::RESET_LIGHT));
	stepsParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(134.0f, 52.0f), module, BORDL::STEPS_PARAM, 1.0f, 16.0f, 8.0f);
	addParam(stepsParam);

	static const float portX0[4] = {20.0f, 58.0f, 96.0f, 135.0f};
 	addInput(Port::create<PJ301MPort>(Vec(portX0[0], 90.0f), Port::INPUT, module, BORDL::CLOCK_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[1], 90.0f), Port::INPUT, module, BORDL::EXT_CLOCK_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 90.0f), Port::INPUT, module, BORDL::RESET_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[3], 90.0f), Port::INPUT, module, BORDL::STEPS_INPUT));

	rootNoteParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX0[0]-2.0f, 140.0f), module, BORDL::ROOT_NOTE_PARAM, 0.0f, BORDL::NUM_NOTES-0.9f, 0.0f);
	addParam(rootNoteParam);
	scaleParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX0[1]-2.0f, 140.0f), module, BORDL::SCALE_PARAM, 0.0f, BORDL::NUM_SCALES-0.9f, 0.0f);
	addParam(scaleParam);
	gateTimeParam = ParamWidget::create<BidooBlueKnob>(Vec(portX0[2]-2.0f, 140.0f), module, BORDL::GATE_TIME_PARAM, 0.1f, 1.0f, 0.5f);
	addParam(gateTimeParam);
	slideTimeParam = ParamWidget::create<BidooBlueKnob>(Vec(portX0[3]-2.0f, 140.0f), module, BORDL::SLIDE_TIME_PARAM	, 0.1f, 1.0f, 0.2f);
	addParam(slideTimeParam);

	addInput(Port::create<PJ301MPort>(Vec(portX0[0], 180.0f), Port::INPUT, module, BORDL::ROOT_NOTE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[1], 180.0f), Port::INPUT, module, BORDL::SCALE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 180.0f), Port::INPUT, module, BORDL::GATE_TIME_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[3], 180.0f), Port::INPUT, module, BORDL::SLIDE_TIME_INPUT));

	playModeParam = ParamWidget::create<BlueCKD6>(Vec(portX0[0]-1.0f, 230.0f), module, BORDL::PLAY_MODE_PARAM, 0.0f, 4.0f, 0.0f);
	addParam(playModeParam);
	countModeParam = ParamWidget::create<BlueCKD6>(Vec(portX0[1]-1.0f, 230.0f), module, BORDL::COUNT_MODE_PARAM, 0.0f, 4.0f, 0.0f);
	addParam(countModeParam);
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 232.0f), Port::INPUT, module, BORDL::PATTERN_INPUT));
	patternParam = ParamWidget::create<BORDLPatternRoundBlackSnapKnob>(Vec(portX0[3],230.0f), module, BORDL::PATTERN_PARAM, 1.0f, 16.0f, 1.0f);
	addParam(patternParam);

	static const float portX1[8] = {200.0f, 238.0f, 276.0f, 315.0f, 353.0f, 392.0f, 430.0f, 469.0f};

	sensitivityParam = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[0]-24.0f, 32.0f), module, BORDL::SENSITIVITY_PARAM, 0.1f, 1.0f, 1.0f);
	addParam(sensitivityParam);

	for (int i = 0; i < 8; i++) {
		pitchParams[i] = ParamWidget::create<BidooBlueKnob>(Vec(portX1[i]+1.0f, 76.0f), module, BORDL::TRIG_PITCH_PARAM + i, 0.0f, 10.001f, 3.0f);
		addParam(pitchParams[i]);
		pitchRndParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+6.5f, 108.0f), module, BORDL::TRIG_PITCHRND_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(pitchRndParams[i]);
		accentParams[i] = ParamWidget::create<BidooBlueKnob>(Vec(portX1[i]+1.0f, 129.0f), module, BORDL::TRIG_ACCENT_PARAM + i, 0.0f, 10.0f, 0.0f);
		addParam(accentParams[i]);
		rndAccentParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+6.5f, 160.0f), module, BORDL::TRIG_RNDACCENT_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(rndAccentParams[i]);
		{
			BORDLPitchDisplay *displayPitch = new BORDLPitchDisplay();
			displayPitch->module = module;
			displayPitch->box.pos = Vec(portX1[i]+15.0f, 45.0f);
			displayPitch->box.size = Vec(20.0f, 10.0f);
			displayPitch->index = i;
			addChild(displayPitch);
		}
		pulseParams[i] = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX1[i]+1.0f, 216.0f), module, BORDL::TRIG_COUNT_PARAM + i, 1.0f, 8.0f,  1.0f);
		addParam(pulseParams[i]);
		pulseProbParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+6.5f, 247.0f), module, BORDL::TRIG_GATEPROB_PARAM + i, 0.0f, 1.0f,  1.0f);
		addParam(pulseProbParams[i]);
		{
			BORDLPulseDisplay *displayPulse = new BORDLPulseDisplay();
			displayPulse->module = module;
			displayPulse->box.pos = Vec(portX1[i]+15.0f, 197.0f);
			displayPulse->box.size = Vec(20.0f, 10.0f);
			displayPulse->index = i;
			addChild(displayPulse);
		}
		typeParams[i] = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX1[i]+1.0f, 288.0f), module, BORDL::TRIG_TYPE_PARAM + i, 0.0f, 5.0f,  2.0f);
		addParam(typeParams[i]);
		{
			BORDLGateDisplay *displayGate = new BORDLGateDisplay();
			displayGate->module = module;
			displayGate->box.pos = Vec(portX1[i]+5.0f, 275.0f);
			displayGate->box.size = Vec(20.0f, 10.0f);
			displayGate->index = i;
			addChild(displayGate);
		}
		slideParams[i] = ParamWidget::create<LEDButton>(Vec(portX1[i]+7.0f, 320.0f), module, BORDL::TRIG_SLIDE_PARAM + i, 0.0f, 1.0f,  0.0f);
		addParam(slideParams[i]);
		addChild(ModuleLightWidget::create<SmallLight<BlueLight>>(Vec(portX1[i]+13.0f, 326.0f), module, BORDL::SLIDES_LIGHTS + i));
		skipParams[i] = ParamWidget::create<LEDButton>(Vec(portX1[i]+7.0f, 341.0f), module, BORDL::TRIG_SKIP_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(skipParams[i]);
		addChild(ModuleLightWidget::create<SmallLight<BlueLight>>(Vec(portX1[i]+13.0f, 347.0f), module, BORDL::SKIPS_LIGHTS + i));
	}

	addInput(Port::create<PJ301MPort>(Vec(10.0f, 331.0f), Port::INPUT, module, BORDL::EXTGATE1_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(43.0f, 331.0f), Port::INPUT, module, BORDL::EXTGATE2_INPUT));
	addOutput(Port::create<PJ301MPort>(Vec(76.5f, 331.0f), Port::OUTPUT, module, BORDL::GATE_OUTPUT));
	addOutput(Port::create<PJ301MPort>(Vec(109.5f, 331.0f), Port::OUTPUT, module, BORDL::PITCH_OUTPUT));
	addOutput(Port::create<PJ301MPort>(Vec(143.0f, 331.0f), Port::OUTPUT, module, BORDL::ACC_OUTPUT));
}

struct BORDLRandPitchItem : MenuItem {
	BORDLWidget *bordlWidget;
	void onAction(EventAction &e) override {
		for (int i = 0; i < 8; i++){
			int index = BORDL::TRIG_PITCH_PARAM + i;
			auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
			if (it != bordlWidget->params.end())
			{
				auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
	}
};

struct BORDLRandGatesItem : MenuItem {
	BORDLWidget *bordlWidget;
	void onAction(EventAction &e) override {
		for (int i = 0; i < 8; i++){
			int index = BORDL::TRIG_COUNT_PARAM + i;
			auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
			if (it != bordlWidget->params.end())
			{
				auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
		for (int i = 0; i < 8; i++){
				int index = BORDL::TRIG_TYPE_PARAM + i;
				auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
				if (it != bordlWidget->params.end())
				{
					auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
	}
};

struct BORDLRandSlideSkipItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->randomizeSlidesSkips();
	}
};

struct BORDLPitchModeItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->pitchMode = !bordlModule->pitchMode;
	}
	void step() override {
		rightText = bordlModule->pitchMode ? "✔" : "";
		MenuItem::step();
	}
};

struct BORDLPitchQuantizeModeItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->pitchQuantizeMode = !bordlModule->pitchQuantizeMode;
	}
	void step() override {
		rightText = bordlModule->pitchQuantizeMode ? "✔" : "";
		MenuItem::step();
	}
};

struct DisconnectMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->disconnect();
	}
};

struct ResetMenuItem : MenuItem {
	BORDLWidget *bordlWidget;
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		for (int i = 0; i < BORDL::NUM_PARAMS; i++){
			if (i != BORDL::PATTERN_PARAM) {
				auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&i](const ParamWidget* m) -> bool { return m->paramId == i; });
				if (it != bordlWidget->params.end())
				{
					auto index = std::distance(bordlWidget->params.begin(), it);
					bordlWidget->params[index]->setValue(bordlWidget->params[index]->defaultValue);
				}
			}
		}
		bordlModule->updateFlag = false;
		bordlModule->reset();
		bordlModule->playMode = 0;
		bordlModule->countMode = 0;
		bordlModule->updateFlag = true;
	}
};

struct RandomizeMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->randomize();
	}
};

struct CloneMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		gRackWidget->cloneModule(moduleWidget);
	}
};

struct DeleteMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		gRackWidget->deleteModule(moduleWidget);
		moduleWidget->finalizeEvents();
		delete moduleWidget;
	}
};

struct BORDLCopyItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->copyPattern = bordlModule->selectedPattern;
	}
};

struct BORDLPasteItem : MenuItem {
	BORDL *bordlModule;
	BORDLWidget *bordlWidget;
	void onAction(EventAction &e) override {
		if (bordlModule && bordlWidget && (bordlModule->copyPattern != bordlModule->selectedPattern) && bordlModule->updateFlag)
		{
			bordlModule->updateFlag = false;
			bordlWidget->stepsParam->setValue(bordlModule->patterns[bordlModule->copyPattern].numberOfStepsParam);
			bordlWidget->rootNoteParam->setValue(bordlModule->patterns[bordlModule->copyPattern].rootNote);
			bordlWidget->scaleParam->setValue(bordlModule->patterns[bordlModule->copyPattern].scale);
			bordlWidget->gateTimeParam->setValue(bordlModule->patterns[bordlModule->copyPattern].gateTime);
			bordlWidget->slideTimeParam->setValue(bordlModule->patterns[bordlModule->copyPattern].slideTime);
			bordlWidget->sensitivityParam->setValue(bordlModule->patterns[bordlModule->copyPattern].sensitivity);
			bordlModule->playMode = bordlModule->patterns[bordlModule->copyPattern].playMode;
			bordlModule->countMode = bordlModule->patterns[bordlModule->copyPattern].countMode;
			for (int i = 0; i < 8; i++) {
				bordlWidget->pitchParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pitch);
				bordlWidget->pulseParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pulsesParam);
				bordlWidget->typeParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].type);
				bordlModule->skipState[i] = bordlModule->patterns[bordlModule->copyPattern].steps[i].skipParam ? 't' : 'f';
				bordlModule->slideState[i] = bordlModule->patterns[bordlModule->copyPattern].steps[i].slide ? 't' : 'f';
				bordlWidget->pulseProbParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].gateProb);
				bordlWidget->pitchRndParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pitchRnd);
				bordlWidget->accentParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].accent);
				bordlWidget->rndAccentParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].rndAccent);
			}
			bordlModule->updateFlag = true;
		}
	}
};

Menu *BORDLWidget::createContextMenu() {
	BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this);
	assert(bordlWidget);

	BORDL *bordlModule = dynamic_cast<BORDL*>(module);
	assert(bordlModule);

	Menu *menu = gScene->createMenu();

	MenuLabel *menuLabel = new MenuLabel();
	menuLabel->text = model->author + " " + model->name;
	menu->addChild(menuLabel);

	ResetMenuItem *resetItem = new ResetMenuItem();
	resetItem->text = "Initialize";
	resetItem->rightText = "+I";
	resetItem->bordlWidget = this;
	resetItem->bordlModule = bordlModule;
	menu->addChild(resetItem);

	DisconnectMenuItem *disconnectItem = new DisconnectMenuItem();
	disconnectItem->text = "Disconnect cables";
	disconnectItem->moduleWidget = this;
	menu->addChild(disconnectItem);

	CloneMenuItem *cloneItem = new CloneMenuItem();
	cloneItem->text = "Duplicate";
	cloneItem->rightText = "+D";
	cloneItem->moduleWidget = this;
	menu->addChild(cloneItem);

	DeleteMenuItem *deleteItem = new DeleteMenuItem();
	deleteItem->text = "Delete";
	deleteItem->rightText = "Backspace/Delete";
	deleteItem->moduleWidget = this;
	menu->addChild(deleteItem);

	MenuLabel *spacerLabel = new MenuLabel();
	menu->addChild(spacerLabel);

	BORDLRandPitchItem *randomizePitchItem = new BORDLRandPitchItem();
	randomizePitchItem->text = "Randomize pitch";
	randomizePitchItem->bordlWidget = bordlWidget;
	menu->addChild(randomizePitchItem);

	BORDLRandGatesItem *randomizeGatesItem = new BORDLRandGatesItem();
	randomizeGatesItem->text = "Randomize gates";
	randomizeGatesItem->bordlWidget = bordlWidget;
	menu->addChild(randomizeGatesItem);

	BORDLRandSlideSkipItem *randomizeSlideSkipItem = new BORDLRandSlideSkipItem();
	randomizeSlideSkipItem->text = "Randomize slides and skips";
	randomizeSlideSkipItem->bordlModule = bordlModule;
	menu->addChild(randomizeSlideSkipItem);

	MenuLabel *spacerLabel2 = new MenuLabel();
	menu->addChild(spacerLabel2);

	BORDLCopyItem *copyItem = new BORDLCopyItem();
	copyItem->text = "Copy pattern";
	copyItem->bordlModule = bordlModule;
	menu->addChild(copyItem);

	BORDLPasteItem *pasteItem = new BORDLPasteItem();
	pasteItem->text = "Paste pattern";
	pasteItem->bordlModule = bordlModule;
	pasteItem->bordlWidget = bordlWidget;
	menu->addChild(pasteItem);

	MenuLabel *spacerLabel3 = new MenuLabel();
	menu->addChild(spacerLabel3);

	BORDLPitchModeItem *pitchModeItem = new BORDLPitchModeItem();
	pitchModeItem->text = "Pitch mode continuous/triggered";
	pitchModeItem->bordlModule = bordlModule;
	menu->addChild(pitchModeItem);

	BORDLPitchQuantizeModeItem *pitchQuantizeModeItem = new BORDLPitchQuantizeModeItem();
	pitchQuantizeModeItem->text = "Pitch full quantize";
	pitchQuantizeModeItem->bordlModule = bordlModule;
	menu->addChild(pitchQuantizeModeItem);

	return menu;
}

Model *modelBORDL = Model::create<BORDL, BORDLWidget>("Bidoo", "bordL", "bordL sequencer", SEQUENCER_TAG);
