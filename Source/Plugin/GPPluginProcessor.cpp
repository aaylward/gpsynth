/*
  ===================================

  Evolutionary Synthesis Audio Plugin

  ===================================
*/

#include "GPPluginProcessor.h"
#include "GPPluginEditor.h"

// DECLARE THAT THIS IS THE PLUGIN CLASS WE WANT TO CREATE
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GeneticProgrammingSynthesizerAudioProcessor();
}

/*
	GP Synthesiser Sound
*/
class GPSound : public SynthesiserSound
{
public:
    GPSound() {}

    bool appliesToNote (const int /*midiNoteNumber*/)           {
        return true;
    }
    bool appliesToChannel (const int /*midiChannel*/)           {
        return true;
    }
};


/*
	GP Synthesiser Voice
		Delete and make new one if render settings change
*/
class GPVoice  : public SynthesiserVoice
{
public:

    GPVoice(GPNetwork* net, unsigned maxnumframes, float* frametimes, unsigned numbufferframes, unsigned numtailframes, unsigned numvars)
        : network(net),
		  maxNumFrames(maxnumframes), frameTimes(frametimes), numBufferFrames(numbufferframes), buffer(nullptr),
		  numTailFrames(numtailframes), tailBuffer(nullptr), forceTailFrameNumber(maxNumFrames - numTailFrames),
		  numVariables(numvars), variables(nullptr),
		  playing(false)
    {
		buffer = (float*) malloc(sizeof(float) * numBufferFrames);
		tailBuffer = (float*) malloc(sizeof(float) * numTailFrames);
		variables = (float*) malloc(sizeof(float) * numVariables);
		fillTail(0);
    }
	
	~GPVoice()
	{
		free(variables);
		free(tailBuffer);
		free(buffer);
	}

	void fillTail(unsigned type)
	{
		if (type == 0) {
			for (unsigned i = 0; i < numTailFrames; i++) {
				tailBuffer[i] = i * (-1.0f / numTailFrames) + 1.0f;
			}
			//appendToTextFile("./debug.txt", "tail0: " + String(tailBuffer[0]) + " tail n/2: " + String(tailBuffer[numTailFrames/2]) + " tail n-1: " + String(tailBuffer[numTailFrames -1]) + "\n");
		}
	}

    void startNote (const int midiNoteNumber, const float velocity,
                    SynthesiserSound* /*sound*/, const int /*currentPitchWheelPosition*/)
	{
		// appendToTextFile("./debug.txt", "startNote: " + String(network->ID) + "\n");
		// fill current note info
		playing = true;
		frameNumber = 0;
		level = velocity * 0.15;

		// fill current render block info
		bufferIndex = 0;
		numRequestedFrames = 0;

		// fill current tail info
		tailBufferIndex = 0;
		userTailInProgress = false;
		forcedTailInProgress = false;

		// fill note info
		variables[0] = (float) MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    }

    void stopNote (const bool allowTailOff)
    {
        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.

			if (playing && !forcedTailInProgress) {
				userTailInProgress = true;
			}
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..
			playing = false;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (const int /*newValue*/)
    {
        // TODO
    }

    void controllerMoved (const int /*controllerNumber*/, const int /*newValue*/)
    {
        // TODO
    }

	// MIGHT BE CALLED ON NUMSAMPLES <= NUM BUFFER FRAMES BUT SHOULDNT BE CALLED ON >
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
    {
		//appendToTextFile("./debug.txt", "renderNextBlock: " + String(numBufferFrames) + " " + String(numSamples) + "\n");
		assert(numSamples <= numBufferFrames);
		if (playing) {
			numRequestedFrames = (unsigned) numSamples;
			/*
			appendToTextFile("./debug.txt", "START: "
				 + String(frameNumber) + ", "
				 + String(numBufferFrames) + ", "
				 + String(maxNumFrames) + ", "
				 + String(userTailInProgress) + ", "
				 + String(forcedTailInProgress) + ", "
				 + String(forceTailFrameNumber) + ", "
				 + String(tailBufferIndex) + ", "
				 + String(numTailFrames) + "\n");
				 */
			// fill audio buffers
			// if we are going at the end of our sample times within this render block
			if (frameNumber + numRequestedFrames > maxNumFrames) {
				network->evaluateBlockPerformance(frameNumber, maxNumFrames - frameNumber, frameTimes + frameNumber, numVariables, variables, buffer);
				frameNumber += maxNumFrames - frameNumber;
			}
			// else fill render block normally
			else {
				network->evaluateBlockPerformance(frameNumber, numRequestedFrames, frameTimes + frameNumber, numVariables, variables, buffer);
				frameNumber += numRequestedFrames;
			}

			// apply MIDI velocity and possible tail
			// if we are in the middle of a tail render
			if (userTailInProgress || forcedTailInProgress) {
				applyTail();
			}
			// else if we will need to begin a force tail render in this block
			// TODO: should this be GREQ?
			else if (frameNumber >= forceTailFrameNumber) {
				unsigned numUntailedFramesInBuffer = numRequestedFrames - (frameNumber - forceTailFrameNumber);
				while (bufferIndex < numUntailedFramesInBuffer) {
					buffer[bufferIndex] = buffer[bufferIndex] * level;
					bufferIndex++;
				}
				forcedTailInProgress = true;
				applyTail();
			}
			// else no tail so just apply MIDI velocity
			else {
				while (bufferIndex < numSamples) {
					buffer[bufferIndex] = buffer[bufferIndex] * level;
					bufferIndex++;
				}
			}

			// reset buffer index because we're done writing to it
			bufferIndex = 0;

			// copy to all outputs
			// TODO: this is the reason I dont have polyphony. Need to look at example plugin
			// and copy it. Need to add buffer to outputBuffer instead of memcpy'ing it ; ;
			for (int i = 0; i < outputBuffer.getNumChannels(); i++) {
				float* chanBuff = outputBuffer.getSampleData(i, startSample);
				for (int j = 0; j < numSamples; j++) {
					chanBuff[j] += buffer[j];
				}
				//memcpy(outputBuffer.getSampleData(i, startSample), buffer, sizeof(float) * numRequestedFrames);
			}
			/*
			appendToTextFile("./debug.txt", "END: "
				 + String(frameNumber) + ", "
				 + String(numBufferFrames) + ", "
				 + String(maxNumFrames) + ", "
				 + String(userTailInProgress) + ", "
				 + String(forcedTailInProgress) + ", "
				 + String(forceTailFrameNumber) + ", "
				 + String(tailBufferIndex) + ", "
				 + String(numTailFrames) + "\n");
				 */
		}
    }

	// all of the following are possible:
	//		anywhere in the middle of the tail
	//		anywhere in the middle of the buffer
	//		buffer may run out before tail does
	//		tail my run out before buffer does
	// have to be in the middle of applying a tail
	inline void applyTail() {
		// find out how many remain of the tail and this render block
		unsigned numTailFramesRemaining = numTailFrames - tailBufferIndex;
		unsigned numBufferFramesRemaining = numRequestedFrames - bufferIndex;
		//appendToTextFile("./debug.txt", "APPLY TAIL START: " + String(numTailFramesRemaining) + ", " + String(numBufferFramesRemaining) + "\n");
		// if our buffer is going to run out before our tail
		if (numTailFramesRemaining > numBufferFramesRemaining) {
			//appendToTextFile("./debug.txt", "1: " + String(bufferIndex) + ", " + String(tailBufferIndex) + "\n");
			while (bufferIndex < numRequestedFrames) {
				buffer[bufferIndex] = buffer[bufferIndex] * level * tailBuffer[tailBufferIndex];
				bufferIndex++;
				tailBufferIndex++;
			}
		}

		// else our tail is going to run out before our buffer
		else {
			//appendToTextFile("./debug.txt", "2: " + String(bufferIndex) + ", " + String(tailBufferIndex) + "\n");
			while (tailBufferIndex < numTailFrames) {
				buffer[bufferIndex] = buffer[bufferIndex] * level * tailBuffer[tailBufferIndex];
				bufferIndex++;
				tailBufferIndex++;
			}
			while (bufferIndex < numRequestedFrames) {
				buffer[bufferIndex] = 0.0;
				bufferIndex++;
			}
			// because our notes will always have a tail, this is the only time playing can be set to false
			playing = false;
			clearCurrentNote();
		}
		//appendToTextFile("./debug.txt", "3: " + String(bufferIndex) + ", " + String(tailBufferIndex) + "\n");
		//appendToTextFile("./debug.txt", "APPLY TAIL END: " + String(numTailFramesRemaining) + ", " + String(numBufferFramesRemaining) + "\n");
	}

    bool canPlaySound (SynthesiserSound* sound)
    {
        return dynamic_cast <GPSound*> (sound) != 0;
    }

private:
	// algorithm
    GPNetwork* network;

	// render information
	unsigned maxNumFrames;
    float* frameTimes;
	unsigned numBufferFrames;
	float* buffer;

	// tail information
	unsigned numTailFrames;
	float* tailBuffer;
	unsigned forceTailFrameNumber;
	
	// control variables
    unsigned numVariables;
    float* variables;

	// current note info
	bool playing;
	unsigned frameNumber;
    float level;

	// current render block info
	unsigned bufferIndex;
	unsigned numRequestedFrames;

	// current tail info
	unsigned tailBufferIndex;
	bool userTailInProgress;
	bool forcedTailInProgress;
};


/*
   ============
   CONSTRUCTION
   ============
*/

GeneticProgrammingSynthesizerAudioProcessor::GeneticProgrammingSynthesizerAudioProcessor()
	:	lastUIWidth(400), lastUIHeight(400),
		fileBrowserOpen(false), synthFolder(File::nonexistent),
		algorithm(0), algorithmFitness(0), gain(1.0f), fitnesses((double*) malloc(sizeof(double) * POPULATIONSIZE)),
		samplerate(0), numsamplesperblock(0), taillengthseconds(TAILLEN),
		numvariables(1),
		maxnoteleninseconds(MAXNOTELEN), maxnumframes(0), allvoicessampletimes(nullptr),
		seed(time(NULL)), rng(seed), params((GPParams*) malloc(sizeof(GPParams))), currentPrimitives(nullptr), generationNum(0), gpsynth(nullptr),
		currentAlgorithm(nullptr), currentGeneration(POPULATIONSIZE, nullptr),
		generationActive(false), algorithmPrepared(false),
		numSynthVoicesPerAlgorithm(NUMVOICES), currentCopies(nullptr), currentGenerationCopies(POPULATIONSIZE, nullptr),
		prevGenerationPressed(false), nextGenerationPressed(false), algorithmLastTimer(algorithm)
{
	// remaining from default plugin
	lastPosInfo.resetToDefault();
	
	//saveTextFile("./debug.txt", String::empty);

    // set up default fitnesses
	for (unsigned i = 0; i < POPULATIONSIZE; i++) {
		fitnesses[i] = 0.0f;
	}

	// set up genetic algorithm
	// params
	// synth and experiment
	params->verbose = false;
	params->savePrecision = 20;
	params->printPrecision = 1;
	params->saveGenerationChampions = false;
	params->lowerFitnessIsBetter = false;
	params->bestPossibleFitness = 1.0f;

    // synth evolution params
    params->populationSize = POPULATIONSIZE;
    params->backupAllNetworks = true;
    params->backupPrecision = 100;
    params->lowerFitnessIsBetter = false; //should be done in experiment
    params->bestPossibleFitness = 2.0; //should be done in experiment
    params->maxInitialHeight = 3;
    params->maxHeight = 5;

    // synth genetic params
    params->proportionOfPopulationForGreedySelection = 0.0;
    // numeric mutation
    params->numericMutationSelectionType = 1;
    params->proportionOfPopulationFromNumericMutation = 0.00;
    params->percentileOfPopulationToSelectFromForNumericMutation = 0.05;
    params->numericMutationTemperatureConstant = 0.9;
    // mutation
    params->mutationSelectionType = 1;
    params->mutationType = 0;
    params->proportionOfPopulationFromMutation = 0.00;
    params->percentileOfPopulationToSelectFromForMutation = 0.05;
    // crossover
    params->crossoverSelectionType = 0;
    params->crossoverType = 0;
    params->proportionOfPopulationFromCrossover = 1.0;
    // reproduction
    params->reproductionSelectionType = 0;
    params->proportionOfPopulationFromReproduction = 0.00;

    // value node params
    params->valueNodeMinimum = -1.0;
    params->valueNodeMaximum = 1.0;

    // oscil node params
    params->oscilNodeMaxPartial = 30;
    params->oscilNodeMinIndexOfModulation = 0.0;
    params->oscilNodeMaxIndexOfModulation = 2.0;

    // modulation node
    params->LFONodeFrequencyRange = 10;

    // delay node
    params->delayNodeBufferMaxSeconds = 1.0;

    // filter node
    params->filterNodeCenterFrequencyMinimum = 20;
    params->filterNodeCenterFrequencyMaximumProportionOfNyquist = 0.9999;
    params->filterNodeQualityMinimum = 0.1;
    params->filterNodeQualityMaximum = 10.0;
    params->filterNodeBandwidthMinimum = 10;
    params->filterNodeBandwidthMaximum = 2000;

    // adsr node
    params->ADSRNodeEnvelopeMin = 0.0;
    params->ADSRNodeEnvelopeMax = 1.0;

	// primitives
	currentPrimitives = new std::vector<GPNode*>(0);
	currentPrimitives->push_back(createNode("(gain {c -1.0 0.0 1.0} (null))", &rng));
	currentPrimitives->push_back(createNode("(const {c -1.0 0.0 1.0})", &rng));
	currentPrimitives->push_back(createNode("(* (null) (null))", &rng));
	currentPrimitives->push_back(createNode("(sinosc {d 0 0 0} {c 0.5 1.0 10.0} {c 0.0 0.0 1.0})", &rng));
	currentPrimitives->push_back(createNode("(sawosc {d 0 0 0} {c 0.5 1.0 10.0} {c 0.0 0.0 1.0})", &rng));
	currentPrimitives->push_back(createNode("(squareosc {d 0 0 0} {c 0.5 1.0 10.0} {c 0.0 0.0 1.0})", &rng));
	currentPrimitives->push_back(createNode("(triangleosc {d 0 0 0} {c 0.5 1.0 10.0} {c 0.0 0.0 1.0})", &rng));
	currentPrimitives->push_back(createNode("(+ (null) (null))", &rng));
	currentPrimitives->push_back(createNode("(lfo* {c 0 0.0 20} (null))", &rng));
	currentPrimitives->push_back(createNode("(am {d 0 0 1} {d 1 1 10} {c 0 0 1.0} {c 0 0 1.0} (null))", &rng));
	currentPrimitives->push_back(createNode("(pm {d 0 0 1} {d 1 1 10} {c 0 1.0 5.0} (null))", &rng));
	currentPrimitives->push_back(createNode("(time)", &rng));
	currentPrimitives->push_back(createNode("(switch (null) (null) (null))", &rng));

	// create synth
	gpsynth = new GPSynth(params, &rng, currentPrimitives);

	// networks and population
	fillFromGeneration();

	// init synth
	synth.addSound(new GPSound());

	// start algorithm change timer
	startTimer(300);
}

GeneticProgrammingSynthesizerAudioProcessor::~GeneticProgrammingSynthesizerAudioProcessor()
{
	releaseResources();
	deleteGenerationState();
	clearSampleTimes();
	delete gpsynth;
	free(params);
	free(fitnesses);
}

/*
   ======================
   AUDIO ENGINE OVERRIDES
   ======================
*/

void GeneticProgrammingSynthesizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	//debugPrint("processor prepareToPlay: " + String(sampleRate) + " " + String(samplesPerBlock) + "\n");

    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    synth.setCurrentPlaybackSampleRate (sampleRate);

	// set render info
	if (samplerate != sampleRate) {
		samplerate = (float) sampleRate;
		updateSampleTimes();
	}
	numsamplesperblock = (unsigned) samplesPerBlock;
	numvariables = 1;

	// prepare copies for rendering and add voices
	for (unsigned i = 0; i < numSynthVoicesPerAlgorithm; i++) {
		currentCopies[i]->prepareToRender(samplerate, numsamplesperblock, maxnumframes, maxnoteleninseconds);
		synth.addVoice(new GPVoice(currentCopies[i], maxnumframes, allvoicessampletimes[i], numsamplesperblock, (unsigned) (taillengthseconds * samplerate), numvariables));
	}
	algorithmPrepared = true;

    keyboardState.reset();
    //delayBuffer.clear();
}

void GeneticProgrammingSynthesizerAudioProcessor::releaseResources()
{
	//debugPrint("processor releaseResources: " + String(samplerate) + "\n");

	// remove old voices
	if (algorithmPrepared) {
		for (unsigned i = 0; i < numSynthVoicesPerAlgorithm; i++) {
			currentCopies[i]->doneRendering();
		}

		/*
		for (unsigned i = 0; i < numSynthVoicesPerAlgorithm; i++) {
			synth.removeVoice(i);
		}
		*/
		synth.clearVoices();
	}
	algorithmPrepared = false;

    keyboardState.reset();
}

void GeneticProgrammingSynthesizerAudioProcessor::reset()
{
	//debugPrint("processor reset: " + String(samplerate) + "\n");
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
    //delayBuffer.clear();
}

void GeneticProgrammingSynthesizerAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    int channel, dp = 0;

    // Now pass any incoming midi messages to our keyboard state object, and let it
    // add messages to the buffer if the user is clicking on the on-screen keys
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    // and now get the synth to process these midi events and generate its output.
	if (algorithmPrepared)
		synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

	// apply the gain
	buffer.applyGain(gain);

	// Go through the incoming data, and apply our gain to it...
	/*
    for (channel = 0; channel < getNumInputChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
		*/

	/*
    // Apply our delay effect to the new output..
    for (channel = 0; channel < getNumInputChannels(); ++channel)
    {
        float* channelData = buffer.getSampleData (channel);
        float* delayData = delayBuffer.getSampleData (jmin (channel, delayBuffer.getNumChannels() - 1));
        dp = delayPosition;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = channelData[i];
            channelData[i] += delayData[dp];
            delayData[dp] = (delayData[dp] + in) * delay;
            if (++dp >= delayBuffer.getNumSamples())
                dp = 0;
        }
    }

    delayPosition = dp;
	*/

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // ask the host for the current time so we can display it...
    AudioPlayHead::CurrentPositionInfo newTime;

    if (getPlayHead() != nullptr && getPlayHead()->getCurrentPosition (newTime))
    {
        // Successfully got the current time from the host..
        lastPosInfo = newTime;
    }
    else
    {
        // If the host fails to fill-in the current time, we'll just clear it to a default..
        lastPosInfo.resetToDefault();
    }

	// clear the midi messages buffer because we're not producing output
	midiMessages.clear();
}

/*
   =====================
   CUSTOM PLUGIN METHODS
   =====================
*/

void GeneticProgrammingSynthesizerAudioProcessor::updateSampleTimes() {
	// remove old sample time information
	clearSampleTimes();

	// update new max num frames from new maxnotelen or new samplerate
	maxnumframes = (unsigned) maxnoteleninseconds * samplerate;
	
	// make sample timings for each voice to avoid concurrency issues
	allvoicessampletimes = (float**) malloc(sizeof(float*) * numSynthVoicesPerAlgorithm);
	for (unsigned i = 0; i < numSynthVoicesPerAlgorithm; i++) {
		allvoicessampletimes[i] = (float*) malloc(sizeof(float) * maxnumframes);
		for (unsigned j = 0; j < maxnumframes; j++) {
			allvoicessampletimes[i][j] = (float) (j / (double) samplerate);
			//debugPrint(String(allvoicessampletimes[i][j]) + "\n");
		}
	}

	// correct rounding errors on maxnoteleninseconds
	if (numSynthVoicesPerAlgorithm > 0 && maxnumframes > 0)
		maxnoteleninseconds = allvoicessampletimes[0][maxnumframes - 1];

	// reset algorithm if it was previously prepared
	if (algorithmPrepared)
		setAlgorithm();
}

void GeneticProgrammingSynthesizerAudioProcessor::clearSampleTimes() {
	// free all previously used memory if nonempty
	if (allvoicessampletimes != nullptr) {
		for (unsigned i = 0; i < numSynthVoicesPerAlgorithm; i++) {
			free(allvoicessampletimes[i]);
		}
		free(allvoicessampletimes);
	}
}

void GeneticProgrammingSynthesizerAudioProcessor::changeNumVariables(unsigned newnumvar) {
	numvariables = newnumvar;
	if (algorithmPrepared)
		setAlgorithm();
}

// custom methods
void GeneticProgrammingSynthesizerAudioProcessor::setAlgorithm() {
	//debugPrint("setAlgorithm called: " + String(algorithm) + "\n");
	// clear out old voices
	releaseResources();

	// update current
	currentAlgorithm = currentGeneration[algorithm];
	currentCopies = currentGenerationCopies[algorithm];

	// update fitness slider
	algorithmFitness = fitnesses[algorithm];

	// add new voices
	// if an algorithm was previously prepared
	if (samplerate != 0 && maxnoteleninseconds != 0) {
		// prepare the new one because we know sample rate is nonzero
		prepareToPlay(samplerate, numsamplesperblock);
	}
	// otherwise prepareToPlay will be called automatically soon
}

void GeneticProgrammingSynthesizerAudioProcessor::fillFromGeneration() {
	// delete the memory from the previous generation
	deleteGenerationState();

	// get the next batch of victims
	gpsynth->getIndividuals(currentGeneration);

	// fill synthesizer voices
	//debugPrint("----- GENERATION " + String(generationNum) + " -----\n");
	for (unsigned i = 0; i < POPULATIONSIZE; i++) {
		currentGenerationCopies[i] = (GPNetwork**) malloc(sizeof(GPNetwork*) * numSynthVoicesPerAlgorithm);
		GPNetwork* net = currentGeneration[i];
		//debugPrint(String(net->ID) + ": " + String(net->toString(3).c_str()) + String("\n"));
		//net->traceNetwork();
		for (unsigned j = 0; j < numSynthVoicesPerAlgorithm; j++) {
			GPNetwork* copy = net->getCopy("clone");
			copy->ID = j;
			copy->traceNetwork();
			currentGenerationCopies[i][j] = copy;
		}
	}
	generationActive = true;

	// set the new algorithm
	setAlgorithm();
}

void GeneticProgrammingSynthesizerAudioProcessor::deleteGenerationState() {
	if (generationActive) {
		//debugPrint("got to here\n");
		releaseResources();
		for (unsigned i = 0; i < POPULATIONSIZE; i++) {
			fitnesses[i] = 0.0f;
			for (unsigned j = 0; j < numSynthVoicesPerAlgorithm; j++) {
				delete currentGenerationCopies[i][j];
			}
			free(currentGenerationCopies[i]);
		}
	}
	generationActive = false;
}

GPNetwork* GeneticProgrammingSynthesizerAudioProcessor::getCurrentNetwork() {
	return NULL;
}
void GeneticProgrammingSynthesizerAudioProcessor::randomizeCurrentNetwork() {
	GPNetwork* nu = gpsynth->growNewIndividual(params->maxInitialHeight);
	if (gpsynth->replaceIndividual(currentAlgorithm, nu)) {
		currentGeneration[algorithm] = nu;
		fitnesses[algorithm] = 0.0f;
		for (unsigned j = 0; j < numSynthVoicesPerAlgorithm; j++) {
			delete currentGenerationCopies[algorithm][j];
			GPNetwork* copy = nu->getCopy("clone");
			copy->ID = j;
			copy->traceNetwork();
			currentGenerationCopies[algorithm][j] = copy;
		}
		setAlgorithm();
	}
}

void GeneticProgrammingSynthesizerAudioProcessor::saveCurrentNetwork() {
    FileChooser myChooser ("choose a synthesis algorithm to replace the current algorithm",
                           File::getSpecialLocation (File::currentExecutableFile),
                           FILETYPEREGEX);
	/*
    //WildcardFileFilter wildcardFilter ("*.synth", String::empty, "synth files");
    FileBrowserComponent browser (FileBrowserComponent::saveMode |
									FileBrowserComponent::canSelectFiles,
                                  synthFolder,
								  nullptr, //&wildcardFilter,
                                  nullptr);
    FileChooserDialogBox dialogBox ("choose a location to save the synthesis algorithm",
                                    String::empty,
                                    browser,
                                    false,
                                    Colours::lightgrey);
									*/
	fileBrowserOpen = true;
    if (myChooser.browseForFileToSave(false))
    {
		fileBrowserOpen = false;
        File selectedFile = myChooser.getResult();
		synthFolder = selectedFile.getParentDirectory();
		saveTextFile(selectedFile.getFullPathName() + String(FILETYPE), String(currentAlgorithm->toString(SAVEPRECISION).c_str()));
    }
}

void GeneticProgrammingSynthesizerAudioProcessor::loadReplacingCurrentNetwork() {
    FileChooser myChooser ("choose a synthesis algorithm to replace the current algorithm",
                           File::getSpecialLocation (File::currentExecutableFile),
                           FILETYPEREGEX);
	/*
    WildcardFileFilter wildcardFilter (String(FILETYPEREGEX), String::empty, "synth files");
    FileBrowserComponent browser (FileBrowserComponent::openMode |
									FileBrowserComponent::canSelectFiles,
                                  synthFolder,
                                  &wildcardFilter,
                                  nullptr);
    FileChooserDialogBox dialogBox ("",
                                    String::empty,
                                    browser,
                                    false,
                                    Colours::lightgrey);
									*/

	fileBrowserOpen = true;
    if (myChooser.browseForFileToOpen())
    {
		fileBrowserOpen = false;
        File selectedFile = myChooser.getResult();
		String algorithmText = readTextFromFile(selectedFile.getFullPathName());
		synthFolder = selectedFile.getParentDirectory();
		GPNetwork* nu = new GPNetwork(&rng, algorithmText.toStdString());
		//debugPrint("replacing: " + currentAlgorithm->toString(3) + " with: " + nu->toString(3));
		if (gpsynth->replaceIndividual(currentAlgorithm, nu)) {
			//debugPrint("successful replace\n");
			currentGeneration[algorithm] = nu;
			fitnesses[algorithm] = 0.0f;
			for (unsigned j = 0; j < numSynthVoicesPerAlgorithm; j++) {
				delete currentGenerationCopies[algorithm][j];
				GPNetwork* copy = nu->getCopy("clone");
				copy->ID = j;
				copy->traceNetwork();
				currentGenerationCopies[algorithm][j] = copy;
			}
			setAlgorithm();
		}
    }
}

void GeneticProgrammingSynthesizerAudioProcessor::nextGeneration() {
	nextGenerationPressed = true;
}

void GeneticProgrammingSynthesizerAudioProcessor::prevGeneration() {
	prevGenerationPressed = true;
}

void GeneticProgrammingSynthesizerAudioProcessor::timerCallback() {
	// advance generation if requested in last timer cycle
	if (nextGenerationPressed) {
		nextGenerationPressed = false;
		for (unsigned i = 0; i < POPULATIONSIZE; i++) {
			gpsynth->assignFitness(currentGeneration[i], fitnesses[i]);
		}
		generationNum++;
		fillFromGeneration();
	}
	// revert to last generation if requested in last timer cycle
	else if (prevGenerationPressed) {
		prevGenerationPressed = false;
		if (gpsynth->prevGeneration() != generationNum) {
			generationNum--;
			fillFromGeneration();
		}
	}
	// only set algorithm if algorithm has changed from the last set algorithm but has been constant for two callbacks
	else if (algorithmLastTimer != algorithm) {
		setAlgorithm();
	}

	algorithmLastTimer = algorithm;
}

void GeneticProgrammingSynthesizerAudioProcessor::debugPrint(String dbgmsg) {
	appendToTextFile("./debug.txt", dbgmsg);
}

/*
   ===============
   AUDIO INTERFACE
   ===============
*/

const String GeneticProgrammingSynthesizerAudioProcessor::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String GeneticProgrammingSynthesizerAudioProcessor::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool GeneticProgrammingSynthesizerAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool GeneticProgrammingSynthesizerAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool GeneticProgrammingSynthesizerAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double GeneticProgrammingSynthesizerAudioProcessor::getTailLengthSeconds() const
{
	// TODO: what does this do?
    return taillengthseconds;
}

/*
   ================
   EDITOR OVERRIDES
   ================
*/

AudioProcessorEditor* GeneticProgrammingSynthesizerAudioProcessor::createEditor()
{
    return new GeneticProgrammingSynthesizerAudioProcessorEditor (this);
}

/*
   ====================
   PARAMETERS INTERFACE
   ====================
*/

int GeneticProgrammingSynthesizerAudioProcessor::getNumParameters()
{
    return totalNumParams;
}

float GeneticProgrammingSynthesizerAudioProcessor::getParameter (int index)
{
    // This method will be called by the host, probably on the audio thread, so
    // it's absolutely time-critical. Don't use critical sections or anything
    // UI-related, or anything at all that may block in any way!
    switch (index)
    {
		case algorithmParam:		return (float) algorithm;
		case algorithmFitnessParam:	return algorithmFitness;
		case gainParam:				return gain;
		default:					return 0.0f;
    }
}

void GeneticProgrammingSynthesizerAudioProcessor::setParameter (int index, float newValue)
{
    // This method will be called by the host, probably on the audio thread, so
    // it's absolutely time-critical. Don't use critical sections or anything
    // UI-related, or anything at all that may block in any way!
	// For now JUCE deals with this as [0.0f, 1.0f] from all hosts
    switch (index)
    {
		case algorithmParam:
			if (newValue <= 0.0f) {
				algorithm = 0;
			}
			else if (newValue >= 1.0f) {
				algorithm = POPULATIONSIZE - 1;
			}
			else {
				algorithm = (unsigned) (newValue * POPULATIONSIZE);
			}
			algorithmFitness = fitnesses[algorithm];
			setParameterNotifyingHost(algorithmFitnessParam, algorithmFitness);
			break;
		case algorithmFitnessParam:
			algorithmFitness = newValue;
			fitnesses[algorithm] = (double) newValue;
			break;
		case gainParam:
			gain = newValue;
			break;
		default:
			break;
    }
}

const String GeneticProgrammingSynthesizerAudioProcessor::getParameterName (int index)
{
    switch (index)
    {
		case algorithmParam:		return "algorithm";
		case algorithmFitnessParam:	return "fitness";
		case gainParam:				return "gain";
		default:					break;
    }

    return String::empty;
}

// TODO: wtf is this?
const String GeneticProgrammingSynthesizerAudioProcessor::getParameterText (int index)
{
    return String (getParameter (index), 2);
}

/*
   ========================
   PROGRAM CHANGE INTERFACE
   ========================
*/

/*
   ==============
   MIDI INTERFACE
   ==============
*/

bool GeneticProgrammingSynthesizerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool GeneticProgrammingSynthesizerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

/*
   =======================
   SAVE SETTINGS INTERFACE
   =======================
*/

void GeneticProgrammingSynthesizerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:

    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);
    xml.setAttribute ("gain", gain);

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}

void GeneticProgrammingSynthesizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our parameters..
            lastUIWidth  = xmlState->getIntAttribute ("uiWidth", lastUIWidth);
            lastUIHeight = xmlState->getIntAttribute ("uiHeight", lastUIHeight);


            gain  = (float) xmlState->getDoubleAttribute ("gain", gain);
        }
    }
}