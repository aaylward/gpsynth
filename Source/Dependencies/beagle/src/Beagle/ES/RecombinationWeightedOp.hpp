/*
 *  Open BEAGLE
 *  Copyright (C) 2001-2007 by Christian Gagne and Marc Parizeau
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact:
 *  Laboratoire de Vision et Systemes Numeriques
 *  Departement de genie electrique et de genie informatique
 *  Universite Laval, Quebec, Canada, G1K 7P4
 *  http://vision.gel.ulaval.ca
 *
 */

/*!
 *  \file   Beagle/ES/RecombinationWeightedOp.hpp
 *  \brief  Definition of the class ES::RecombinationWeightedOp.
 *  \author Christian Gagne
 *  $Revision: 1.4 $
 *  $Date: 2007/08/08 19:26:34 $
 */

#ifndef Beagle_ES_RecombinationWeightedOp_hpp
#define Beagle_ES_RecombinationWeightedOp_hpp

#include <string>

#include "Beagle/Core.hpp"
#include "Beagle/EC.hpp"
#include "Beagle/FltVec.hpp"


namespace Beagle
{

namespace ES
{

/*!
 *  \class RecombinationWeightedOp Beagle/ES/RecombinationWeightedOp.hpp
 *    "Beagle/ES/RecombinationWeightedOp.hpp"
 *  \brief Real-valued recombination by weighted mean operator class.
 *  \ingroup ESF
 */
class RecombinationWeightedOp : public EC::RecombinationOp
{

public:

	//! ES::RecombinationWeightedOp allocator type.
	typedef AllocatorT<RecombinationWeightedOp,EC::RecombinationOp::Alloc>
	Alloc;
	//! ES::RecombinationWeightedOp handle type.
	typedef PointerT<RecombinationWeightedOp,EC::RecombinationOp::Handle>
	Handle;
	//! ES::RecombinationWeightedOp bag type.
	typedef ContainerT<RecombinationWeightedOp,EC::RecombinationOp::Bag>
	Bag;

	explicit RecombinationWeightedOp(std::string inRecombPbName="es.rec.prob",
	                                 std::string inNumberRecombName="ES.rec.nbrind",
	                                 std::string inName="ES-RecombinationWeightedOp");
	virtual ~RecombinationWeightedOp()
	{ }

	virtual Individual::Handle recombine(Individual::Bag& inIndivPool, Context& ioContext);

};

}
}

#endif // Beagle_ES_RecombinationWeightedOp_hpp
