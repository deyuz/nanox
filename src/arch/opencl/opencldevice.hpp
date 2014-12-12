
/*************************************************************************************/
/*      Copyright 2013 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#ifndef _OpenCL_DEVICE
#define _OpenCL_DEVICE

#include "opencldevice_decl.hpp"
#include "openclprocessor.hpp" 
#include "deviceops.hpp"

using namespace nanos;
using namespace nanos::ext;

OpenCLDevice::OpenCLDevice( const char *name ) : Device( name ) { }

void *OpenCLDevice::memAllocate( std::size_t size, SeparateMemoryAddressSpace &mem, WorkDescriptor const &wd, unsigned int copyIdx)
{ 
   nanos::ProcessingElement * pe = &(mem.getPE());
   if( OpenCLProcessor *proc = dynamic_cast<OpenCLProcessor *>( pe ) ) {
       CopyData cdata=wd.getCopies()[copyIdx];
       //If we are on allocWide mode and we have the complete size (aka we are allocating the whole structure), do offset = 0
       if (nanos::ext::OpenCLConfig::getAllocWide() && cdata.getSize()!=cdata.getMaxSize()) {
         return proc->allocate( size , cdata.getAddress(), 0);
       } else {
         return proc->allocate( size , cdata.getAddress(), cdata.getOffset());
       }
   }


   fatal( "Can allocate only on OpenCLProcessor" );
}

//void *OpenCLDevice::realloc( void * address,
//                          size_t size,
//                          size_t ceSize,
//                          ProcessingElement *pe )
//{
//   if( OpenCLProcessor *proc = dynamic_cast<OpenCLProcessor *>( pe ) )
//      return proc->realloc( address, size, ceSize );
//   fatal( "Can reallocate only on OpenCLProcessor" );
//}

void OpenCLDevice::memFree( uint64_t addr, SeparateMemoryAddressSpace &mem )
{
    nanos::ProcessingElement * pe = &(mem.getPE());

    if( OpenCLProcessor *proc = dynamic_cast<OpenCLProcessor *>( pe ) )
      return proc->free( (void*) addr );


   fatal( "Can free only on OpenCLProcessor" );
}

void OpenCLDevice::_copyIn( uint64_t devAddr, uint64_t hostAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const
{
   nanos::ProcessingElement * pe = &(mem.getPE());
   if( OpenCLProcessor *proc = dynamic_cast<OpenCLProcessor *>( pe ) )
   {
        proc->copyIn( devAddr, hostAddr, len, ops );
   }
}

void OpenCLDevice::_copyOut( uint64_t hostAddr, uint64_t devAddr, std::size_t len, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const
{
   nanos::ProcessingElement * pe = &(mem.getPE());
   if( OpenCLProcessor *proc = dynamic_cast<OpenCLProcessor *>( pe ) )
   {
        proc->copyOut( hostAddr, devAddr, len, ops );
   }
}

bool OpenCLDevice::_copyDevToDev( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, SeparateMemoryAddressSpace &memDest, SeparateMemoryAddressSpace &memOrig, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const
{
    //If user disabled devToDev copies (sometimes they give bad performance...)
    if (nanos::ext::OpenCLConfig::getDisableDev2Dev()) return false;
    nanos::ext::OpenCLProcessor *procDst = (nanos::ext::OpenCLProcessor *)( &(memDest.getPE()) );
    nanos::ext::OpenCLProcessor *procSrc = (nanos::ext::OpenCLProcessor *)( &(memOrig.getPE()) );
    //If both devices are in the same vendor/context do a real copy in       
   //If shared memory, no need to copy (I hope, all OCL devices should share the same memory space...)
    if (procDst->getContext()==procSrc->getContext() && !OpenCLProcessor::getSharedMemAllocator().isSharedMem( (void*) devOrigAddr, len)) {       
        cl_mem buf=procSrc->getBuffer( (void*) devOrigAddr, len);
        procDst->copyInBuffer( (void*) devDestAddr, buf, len, ops);
        //TODO: Check this functor
        if ( f ) {
           (*f)(); 
        }
        return true;
    }
    return false;
}


void OpenCLDevice::_getFreeMemoryChunksList( SeparateMemoryAddressSpace const &mem, SimpleAllocator::ChunkList &list ) const {
    nanos::ext::OpenCLProcessor const *pe = (nanos::ext::OpenCLProcessor const *)&(mem.getConstPE());
    pe->getConstCacheAllocator().getFreeChunksList(list);
}

std::size_t OpenCLDevice::getMemCapacity( SeparateMemoryAddressSpace const &mem ) const {
    nanos::ext::OpenCLProcessor const *pe = (nanos::ext::OpenCLProcessor const *)&(mem.getConstPE());
    return pe->getConstCacheAllocator().getCapacity();
}

void OpenCLDevice::_canAllocate( SeparateMemoryAddressSpace const &mem, std::size_t *sizes, unsigned int numChunks, std::size_t *remainingSizes ) const { }

void OpenCLDevice::_copyInStrided1D( uint64_t devAddr, uint64_t hostAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace const &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
   fatal("Error: " << __PRETTY_FUNCTION__ << " is not implemented.");
}

void OpenCLDevice::_copyOutStrided1D( uint64_t hostAddr, uint64_t devAddr, std::size_t len, std::size_t count, std::size_t ld, SeparateMemoryAddressSpace &mem, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) {
   fatal("Error: " << __PRETTY_FUNCTION__ << " is not implemented.");
}

bool OpenCLDevice::_copyDevToDevStrided1D( uint64_t devDestAddr, uint64_t devOrigAddr, std::size_t len, std::size_t numChunks, std::size_t ld, SeparateMemoryAddressSpace const &memDest, SeparateMemoryAddressSpace const &memOrig, DeviceOps *ops, Functor *f, WD const &wd, void *hostObject, reg_t hostRegionId ) const {
   fatal("Error: " << __PRETTY_FUNCTION__ << " is not implemented.");
}

#endif // _OpenCL_DEVICE
