// $Id: SOAPMessenger.h,v 1.2 2011/01/25 17:36:47 banicz Exp $

/*************************************************************************
 * XDAQ Components for Distributed Data Acquisition                      *
 * Copyright (C) 2000-2009, CERN.			                 *
 * All rights reserved.                                                  *
 * Authors: J. Gutleber and L. Orsini					 *
 *                                                                       *
 * For the licensing terms see LICENSE.		                         *
 * For the list of contributors see CREDITS.   			         *
 *************************************************************************/

#ifndef _pt_http_SOAPMessenger_h
#define _pt_http_SOAPMessenger_h

#include "xoap/MessageReference.h"
#include "pt/SOAPMessenger.h"
#include "pt/exception/Exception.h"
#include "pt/Address.h"

#include "pt/http/Channel.h"
#include "pt/http/exception/Exception.h"

#include "log4cplus/logger.h"

using namespace log4cplus;

namespace pt
{
namespace http
{
	class SOAPMessenger: public pt::SOAPMessenger
	{
		public:
			
			SOAPMessenger(Logger & logger, pt::Address::Reference destination, pt::Address::Reference local)
			    throw (pt::http::exception::Exception);
			
			//! Destructor only deletes channel object
			//
			virtual ~SOAPMessenger();
			
			pt::Address::Reference getLocalAddress();
			pt::Address::Reference getDestinationAddress();
			
			xoap::MessageReference send (xoap::MessageReference message) throw (pt::exception::Exception);
			
		private:
				
			http::ClientChannel* channel_;			
			pt::Address::Reference local_;
			pt::Address::Reference destination_;
			Logger logger_;
	};

}
}

#endif
