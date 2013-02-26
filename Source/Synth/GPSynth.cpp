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

GPSynth::GPSynth(unsigned psize, bool lowerbetter, double best, unsigned mid, unsigned md, unsigned crosstype, unsigned reproduceselecttype, unsigned crossselecttype, double crosspercent, double addchance, double subchance, double mutatechance, std::vector<GPNode*>* nodes, GPNodeParams* p) :
nextNetworkID(0), generationID(0), currentIndividualNumber(0),
populationSize(psize), lowerFitnessIsBetter(lowerbetter), bestPossibleFitness(best), maxInitialDepth(mid), maxDepth(md), crossoverType(crosstype), reproductionSelectionType(reproduceselecttype), crossoverSelectionType(crossselecttype), crossoverProportion(crosspercent),
nodeAddChance(addchance), nodeRemoveChance(subchance), nodeMutateChance(mutatechance),
availableNodes(nodes), availableFunctions(), availableTerminals(),
allNetworks(), upForEvaluation(), evaluated(),
rawFitnesses(), normalizedFitnesses()
{
    for (int i = 0; i < nodes->size(); i++) {
        if (nodes->at(i)->isTerminal) {
            availableTerminals.push_back(nodes->at(i));
        }
        else {
            availableFunctions.push_back(nodes->at(i));
        }
    }
    nodeParams = p;
    rng = nodeParams->rng;
    std::cout << "Initializing population of size " << populationSize << " with best possible fitness of " << bestPossibleFitness << std::endl;
    initPopulation();
}

GPSynth::~GPSynth() {
}

GPNode* GPSynth::fullRecursive(unsigned cd, GPNode* p, unsigned d) {
    if (cd == d) {
        GPNode* term = availableTerminals[rng->random(availableTerminals.size())]->getCopy();
        term->parent = p;
        return term;
    }
    else {
        GPNode* ret = availableFunctions[rng->random(availableFunctions.size())]->getCopy();
        ret->parent = p;
        ret->left = fullRecursive(cd + 1, ret, d);
        if (ret->isBinary) {
            ret->right = fullRecursive(cd + 1, ret, d);
        }
        return ret;
    }
}

GPNetwork* GPSynth::full(unsigned d) {
    return new GPNetwork(fullRecursive(0, NULL, d));
}

// TODO: fix this so it doesnt fill the trees....
GPNode* GPSynth::growRecursive(unsigned cd, GPNode* p, unsigned m) {
    if (cd == m) {
        GPNode* term = availableTerminals[rng->random(availableTerminals.size())]->getCopy();
        term->parent = p;
        return term;
    }
    else {
        GPNode* ret;
        if (cd == 0) {
            ret = availableFunctions[rng->random(availableFunctions.size())]->getCopy();
        }
        else {
            ret = availableNodes->at(rng->random(availableNodes->size()))->getCopy();
        }
        ret->parent = p;
        if (ret->isTerminal) {
            return ret;
        }
        ret->left = fullRecursive(cd + 1, ret, m);
        if (ret->isBinary) {
            ret->right = fullRecursive(cd + 1, ret, m);
        }
        return ret;
    }
}

GPNetwork* GPSynth::grow(unsigned m) {
    return new GPNetwork(growRecursive(0, NULL, m));
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
    assert(allNetworks.size() == populationSize);
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

    // complicated logic to deal with reproduced networks and ones that need evaluation
    GPNetwork* ret = upForEvaluation[currentIndividualNumber];
    while (ret->fitness != NAN && currentIndividualNumber < populationSize - 1) {
        std::cout << "Algorithm " << ret->ID << " with depth " << ret->getDepth() << " and structure " << ret->toString() << " was reproduced from last generation with a fitness of " << ret->fitness << std::endl;
        ret = upForEvaluation[++currentIndividualNumber];
    }
    if (ret->fitness != NAN) {
        std::cout << "Algorithm " << ret->ID << " with depth " << ret->getDepth() << " and structure " << ret->toString() << " was reproduced from last generation with a fitness of " << ret->fitness << std::endl;
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
    // TODO: fix this for assigned fitnesses and NAN and whatnot
    for (int i = 0; i < populationSize; i++) {
        upForEvaluation.push_back(allNetworks.back());
        allNetworks.erase(allNetworks.end() - 1);
    }
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

    for (int i = 0; i < evaluated.size(); i++) {
        evaluated[i]->isAlive = false;
    }

    unsigned numToReproduce = (unsigned) ((1 - crossoverProportion) * populationSize);

    for (int i = 0; i < numToReproduce; i++) {
      GPNetwork* one = selectFromEvaluated(reproductionSelectionType);
      one->isAlive = true;
      addNetworkToPopulation(one);
    }
    while(upForEvaluation.size() < populationSize) {
      GPNetwork* dad = selectFromEvaluated(crossoverSelectionType);
      GPNetwork* mom = selectFromEvaluated(crossoverSelectionType);
      GPNetwork* one = dad->getCopy();
      GPNetwork* two = dad->getCopy();

      if (nodeParams->rng->random() < nodeMutateChance) {
        one->mutate(nodeParams);
      }
      if (nodeParams->rng->random() < nodeMutateChance) {
        two->mutate(nodeParams);
      }

      GPNetwork* offspring = reproduce(one, two);

      // standard GP with two offspring
      if (offspring == NULL) {
        if (one->getDepth() > maxDepth) {
          delete one;
          one = dad->getCopy();
        }
        addNetworkToPopulation(one, true);
        if (upForEvaluation.size() < populationSize) {
          if (two->getDepth() > maxDepth) {
            delete two;
            two = mom->getCopy();
          }
          addNetworkToPopulation(two, true);
        }
      }
      // some other type with one offspring
      else {
        addNetworkToPopulation(offspring, true);
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

void GPSynth::addNetworkToPopulation(GPNetwork* net, bool retrace) {
    if (net->isAlive == false) {
        net->ID = nextNetworkID++;
        net->isAlive = true;
    }
    allNetworks.push_back(net);
    upForEvaluation.push_back(net);
    if (retrace)
        net->traceNetwork();
}

void GPSynth::clearGenerationState() {
  rawFitnesses.clear();
  normalizedFitnesses.clear();
  rank.clear();
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
        return evaluated[nodeParams->rng->sampleFromDistribution(&normalizedFitnesses)];
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
        return evaluated[(int) (nodeParams->rng->random() * evaluated.size())];
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
        GPNode* subtreeone = one->getRandomNetworkNode(nodeParams->rng);
        GPNode* subtreetwo = two->getRandomNetworkNode(nodeParams->rng);
        one->replaceSubtree(subtreeone, subtreetwo);
        two->replaceSubtree(subtreetwo, subtreeone);

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
        GPNode* newroot = new FunctionNode(multiply, "*", one->getRoot(), two->getRoot());
        GPNetwork* newnet = new GPNetwork(newroot);
        return newnet;
    }
    // experimental array crossover
    else if (crossoverType == 4) {
        return NULL;
    }
    return NULL;
}
