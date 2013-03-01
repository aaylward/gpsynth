/*
  ==============================================================================

    GPSynth.cpp
    Created: 6 Feb 2013 7:19:38pm
    Author:  cdonahue

  ==============================================================================
*/

#include "GPSynth.h"

/*
   ============
   CONSTRUCTION
   ============
*/

GPSynth::GPSynth(GPParams* p, std::vector<GPNode*>* nodes) :
nextNetworkID(0), generationID(0), currentIndividualNumber(0),
populationSize(p->populationSize),
lowerFitnessIsBetter(p->lowerFitnessIsBetter),
bestPossibleFitness(p->bestPossibleFitness),
maxInitialDepth(p->maxInitialDepth),
maxDepth(p->maxDepth),
mutationDuringInitializationChance(p->mutationDuringInitializationChance),
proportionOfPopulationFromCrossover(p->proportionOfPopulationFromCrossover),
reproductionSelectionType(p->reproductionSelectionType),
crossoverType(p->crossoverType),
crossoverSelectionType(p->crossoverSelectionType),
mutationDuringCrossoverChance(p->mutationDuringCrossoverChance),
availableNodes(nodes),
allNetworks(), upForEvaluation(), evaluated(),
rawFitnesses(), normalizedFitnesses()
{
    params = p;
    availableFunctions = new std::vector<GPNode*>();
    availableTerminals = new std::vector<GPNode*>();
    for (int i = 0; i < availableNodes->size(); i++) {
        if (availableNodes->at(i)->isTerminal) {
            availableTerminals->push_back(nodes->at(i));
        }
        else {
            availableFunctions->push_back(nodes->at(i));
        }
    }
    rng = params->rng;
    std::cout << "Initializing population of size " << populationSize << " with best possible fitness of " << bestPossibleFitness << std::endl;
    initPopulation();
}

GPSynth::~GPSynth() {
    for (int i = 0; i < allNetworks.size(); i++) {
        delete allNetworks[i];
    }
    delete availableNodes;
    delete availableFunctions;
    delete availableTerminals;
}

GPNode* GPSynth::fullRecursive(unsigned cd, unsigned d) {
    if (cd == d) {
        GPNode* term = availableTerminals->at(rng->random(availableTerminals->size()))->getCopy();
        return term;
    }
    else {
        GPNode* ret = availableFunctions->at(rng->random(availableFunctions->size()))->getCopy();
        ret->left = fullRecursive(cd + 1, d);
        if (ret->isBinary) {
            ret->right = fullRecursive(cd + 1, d);
        }
        return ret;
    }
}

GPNetwork* GPSynth::full(unsigned d) {
    return new GPNetwork(fullRecursive(0, d));
}

GPNode* GPSynth::growRecursive(unsigned cd, unsigned m) {
    if (cd == m) {
        GPNode* term = availableTerminals->at(rng->random(availableTerminals->size()))->getCopy();
        return term;
    }
    else {
        GPNode* ret;
        if (cd == 0) {
            ret = availableFunctions->at(rng->random(availableFunctions->size()))->getCopy();
        }
        else {
            ret = availableNodes->at(rng->random(availableNodes->size()))->getCopy();
        }
        if (ret->isTerminal) {
            return ret;
        }
        ret->left = growRecursive(cd + 1, m);
        if (ret->isBinary) {
            ret->right = growRecursive(cd + 1, m);
        }
        return ret;
    }
}

GPNetwork* GPSynth::grow(unsigned m) {
    return new GPNetwork(growRecursive(0, m));
}

void GPSynth::initPopulation() {
    // implementation of ramped half and half
    unsigned numPerPart = populationSize / (maxInitialDepth - 1);
    unsigned numFullPerPart = numPerPart / 2;
    unsigned numGrowPerPart = numFullPerPart + (numPerPart % 2);
    unsigned additionalLargest = populationSize % (maxInitialDepth - 1);
    unsigned additionalFull = additionalLargest / 2;
    unsigned additionalGrow = additionalFull + (additionalLargest % 2);

    for (int i = 0; i < maxInitialDepth - 1; i++) {
        for (int j = 0; j < numFullPerPart; j++) {
            addNetworkToPopulation(full(i + 2));
        }
        for (int j = 0; j < numGrowPerPart; j++) {
            addNetworkToPopulation(grow(i + 2));
        }
    }
    for (int j = 0; j < additionalFull; j++) {
        addNetworkToPopulation(full(maxInitialDepth));
    }
    for (int j = 0; j < additionalGrow; j++) {
        addNetworkToPopulation(grow(maxInitialDepth));
    }
    assert(upForEvaluation.size() == populationSize);
}

/*
   =================
   EVOLUTION CONTROL
   =================
*/

GPNetwork* GPSynth::getIndividual() {
    // if no more networks remain advance population
    if (currentIndividualNumber == populationSize) {
        currentIndividualNumber = 0;
        nextGeneration();
    }

    // print generation delimiter
    if (currentIndividualNumber == 0) {
        std::cout << "------------------------- START OF GENERATION " << generationID << " -------------------------" << std::endl;
    }

    // logic to deal with reproduced algorithms for efficiency
    GPNetwork* ret = upForEvaluation[currentIndividualNumber];
    if (ret->fitness != -1) {
        std::cout << "Algorithm " << ret->ID << " with depth " << ret->getDepth() << " and structure " << ret->toString() << " was reproduced from last generation with a fitness of " << ret->fitness << std::endl;
        evaluated.push_back(ret);
        rawFitnesses.push_back(ret->fitness);
        upForEvaluation.at(currentIndividualNumber) = NULL;
        currentIndividualNumber++;
        return getIndividual();
    }
    else {
        std::cout << "Testing algorithm " << ret->ID << " with depth " << ret->getDepth() << " and structure " << ret->toString() << std::endl;
        return ret;
    }
}

int GPSynth::assignFitness(GPNetwork* net, double fitness) {
    bool badPointer = true;
    for (int i = 0; i < upForEvaluation.size(); i++) {
        if (net == upForEvaluation[i]) {
            evaluated.push_back(net);
            rawFitnesses.push_back(fitness);
            upForEvaluation.at(i) = NULL;
            std::cout << "Algorithm " << net->ID << " was assigned fitness " << fitness << std::endl;
            currentIndividualNumber++;
            badPointer = false;
            break;
        }
    }
    if (badPointer) {
        std::cerr << "Assigned fitness for a pointer not in upForEvaluation. Probably tried to assign fitness twice for the same network. This shouldn't happen" << std::endl;
        return -1;
    }
    return populationSize - evaluated.size();
}

int GPSynth::prevGeneration() {
    if (generationID == 0) {
        std::cerr << "Attempted to revert to a previous generation during generation 0" << std::endl;
        return generationID;
    }
    clearGenerationState();
    upForEvaluation.clear();
    currentIndividualNumber = 0;
    /*
        // TODO: implement this, requires GPNetwork(std::string)
    */
    generationID--;
    return generationID;
}

void GPSynth::printGenerationSummary() {
    assert(evaluated.size() == rawFitnesses.size() && evaluated.size() == populationSize);
    double generationCumulativeFitness = 0;
    double generationBestFitness = lowerFitnessIsBetter ? INFINITY : 0;
    GPNetwork* champ = NULL;
    for (int i = 0; i < rawFitnesses.size(); i++) {
        double fitness = rawFitnesses[i];
        if (fitness < 0) {
            std::cerr << "Negative fitness value detected when summarizing generation" << std::endl;
            return;
        }
        generationCumulativeFitness += fitness;
        if (lowerFitnessIsBetter && fitness < generationBestFitness) {
            generationBestFitness = fitness;
            champ = evaluated[i];
        }
        else if (!lowerFitnessIsBetter && fitness > generationBestFitness) {
            generationBestFitness = fitness;
            champ = evaluated[i];
        }
    }
    double generationAverageFitness = generationCumulativeFitness / populationSize;
    std::cout << "Generation " << generationID << " had average fitness " << generationAverageFitness << " and minimum fitness " << generationBestFitness << " attained by the structure " << champ->toString() << std::endl;
}

int GPSynth::nextGeneration() {
    assert(evaluated.size() == rawFitnesses.size() && evaluated.size() == populationSize);
    upForEvaluation.clear();

    unsigned numToReproduce = (unsigned) ((1 - proportionOfPopulationFromCrossover) * populationSize);

    for (int i = 0; i < numToReproduce; i++) {
      GPNetwork* selected = selectFromEvaluated(reproductionSelectionType);
      double oldFitness = selected->fitness;
      GPNetwork* one = selected->getCopy();
      one->fitness = oldFitness;
      addNetworkToPopulation(one);
    }
    while(upForEvaluation.size() < populationSize) {
      GPNetwork* dad = selectFromEvaluated(crossoverSelectionType);
      GPNetwork* mom = selectFromEvaluated(crossoverSelectionType);
      GPNetwork* one = dad->getCopy();
      one->traceNetwork();
      one->ID = dad->ID;
      GPNetwork* two = mom->getCopy();
      two->traceNetwork();
      two->ID = mom->ID;

      if (params->rng->random() < mutationDuringCrossoverChance) {
        one->mutate(params);
      }
      if (params->rng->random() < mutationDuringCrossoverChance) {
        two->mutate(params);
      }

      GPNetwork* offspring = reproduce(one, two);

      // standard GP with two offspring
      if (offspring == NULL) {
        if (one->getDepth() > maxDepth) {
          delete one;
          one = dad->getCopy();
        }
        addNetworkToPopulation(one);
        if (upForEvaluation.size() < populationSize) {
          if (two->getDepth() > maxDepth) {
            delete two;
            two = mom->getCopy();
          }
          addNetworkToPopulation(two);
        }
      }
      // some other type with one offspring
      else {
        addNetworkToPopulation(offspring);
      }
    }

    clearGenerationState();
    generationID++;
    return generationID;
}

/*
   =======
   HELPERS
   =======
*/

void GPSynth::addNetworkToPopulation(GPNetwork* net) {
    net->ID = nextNetworkID++;
    allNetworks.push_back(new std::string(net->toString()));
    upForEvaluation.push_back(net);
    net->traceNetwork();
}

void GPSynth::clearGenerationState() {
  rawFitnesses.clear();
  normalizedFitnesses.clear();
  rank.clear();
  for (int i = 0; i < evaluated.size(); i++) {
    delete evaluated[i];
  }
  evaluated.clear();
}

GPNetwork* GPSynth::selectFromEvaluated(unsigned selectionType) {
    //http://en.wikipedia.org/wiki/Selection_%28genetic_algorithm%29
    if (normalizedFitnesses.size() == 0) {
        // STANDARDIZE FITNESS
        std::vector<double>* standardizedFitnesses;
        if (lowerFitnessIsBetter) {
            standardizedFitnesses = &rawFitnesses;
        }
        else {
            standardizedFitnesses = new std::vector<double>();
            for (int i = 0; i < rawFitnesses.size(); i++) {
                standardizedFitnesses->push_back(bestPossibleFitness - rawFitnesses[i]);
            }
        }

        // TODO: USE STANDARDIZED FITNESSES TO FILL RANK

        // ADJUST FITNESS
        std::vector<double>* adjustedFitnesses = new std::vector<double>();
        double sum = 0;
        double si = 0;
        for (int i = 0; i < standardizedFitnesses->size(); i++) {
            si = 1/(1 + standardizedFitnesses->at(i));
            sum += si;
            adjustedFitnesses->push_back(si);
        }

        // NORMALIZE FITNESS
        for (int i = 0; i < adjustedFitnesses->size(); i++) {
            normalizedFitnesses.push_back(adjustedFitnesses->at(i)/sum);
        }

        // DELETE INTERMEDIATE DATA
        if (!lowerFitnessIsBetter) {
            delete standardizedFitnesses;
        }
        delete adjustedFitnesses;
    }

    if (selectionType == 0) {
        // fitness proportionate selection (lower better)
        return evaluated[params->rng->sampleFromDistribution(&normalizedFitnesses)];
    }
    else if (selectionType == 1) {
        // ranking linear selection
        return NULL;
    }
    else if (selectionType == 2) {
        // ranking curved selection
        return NULL;
    }
    else if (selectionType == 3) {
        // tournament selection
        return NULL;
    }
    else if (selectionType == 4) {
        // stochastic universal sampling selection
        return NULL;
    }
    else if (selectionType == 5) {
        // greedy over selection
        return NULL;
    }
    else if (selectionType == 6) {
        // fitness-unaware random selection
        return evaluated[(int) (params->rng->random() * evaluated.size())];
    }
    return NULL;
}

/*
    =========
    CROSSOVER
    =========
*/

GPNetwork* GPSynth::reproduce(GPNetwork* one, GPNetwork* two) {
    if (crossoverType == 0) {
        // standard GP crossover
        GPNode* subtreeone = one->getRandomNetworkNode(params->rng);
        GPNode* subtreeonecopy = subtreeone->getCopy();
        GPNode* subtreetwo = two->getRandomNetworkNode(params->rng);
        GPNode* subtreetwocopy = subtreetwo->getCopy();
        std::cout << "-----------------" << std::endl;
        std::cout << one->ID << ": " << one->toString() << std::endl;
        std::cout << two->ID << ": " << two->toString() << std::endl;
        std::cout << subtreeonecopy->toString() << std::endl;
        std::cout << subtreetwocopy->toString() << std::endl;
        one->replaceSubtree(subtreeone, subtreetwocopy);
        two->replaceSubtree(subtreetwo, subtreeonecopy);
        // TODO: remove once right
        one->traceNetwork();
        two->traceNetwork();
        std::cout << one->toString() << std::endl;
        std::cout << two->toString() << std::endl;
        delete subtreeone;
        delete subtreetwo;

        return NULL;
    }
    else if (crossoverType == 1) {
        return one;
    }
    else if (crossoverType == 2) {
        return two;
    }
    else if (crossoverType == 3) {
        // AM crossover
        GPNode* newroot = new FunctionNode(multiply, one->getRoot(), two->getRoot());
        GPNetwork* newnet = new GPNetwork(newroot);
        return newnet;
    }
    // experimental array crossover
    else if (crossoverType == 4) {
        return NULL;
    }
    return NULL;
}
