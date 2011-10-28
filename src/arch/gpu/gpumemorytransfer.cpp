/*************************************************************************************/
/*      Copyright 2009 Barcelona Supercomputing Center                               */
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


#include "gpumemorytransfer.hpp"
#include "gpuprocessor.hpp"
#include "instrumentationmodule_decl.hpp"


using namespace nanos;
using namespace nanos::ext;


void GPUMemoryTransferOutList::removeMemoryTransfer ()
{
   if ( !_pendingTransfersAsync.empty() ) {
      bool found = false;
      for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
            it != _pendingTransfersAsync.end(); it++ ) {
         _lock.acquire();
         if ( it->_requested ) {
            found = true;
            GPUMemoryTransfer mt ( *it );
            it = _pendingTransfersAsync.erase( it );
            _lock.release();
            removeMemoryTransfer( mt );
            break;
         }
         _lock.release();
     }

      if ( !found ) {
         _lock.acquire();
         GPUMemoryTransfer mt ( *_pendingTransfersAsync.begin() );
         _pendingTransfersAsync.erase( _pendingTransfersAsync.begin() );
         _lock.release();
         removeMemoryTransfer( mt );
      }
   }
}

void GPUMemoryTransferOutList::checkAddressForMemoryTransfer ( void * address )
{
   for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
         it != _pendingTransfersAsync.end();
         it++ ) {
      _lock.acquire();
      if ( it->_hostAddress.getTag() == ( uint64_t ) address ) {
         GPUMemoryTransfer mt ( *it );
         it = _pendingTransfersAsync.erase( it );
         _lock.release();
         removeMemoryTransfer( mt );
         _lock.acquire();
      }
      _lock.release();
   }
}

void GPUMemoryTransferOutList::requestTransfer( void * address )
{
   _lock.acquire();
   for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
         it != _pendingTransfersAsync.end(); it++ ) {
      if ( it->_hostAddress.getTag() == ( uint64_t ) address ) {
         it->_requested = true;
      }
   }
   _lock.release();
}


void GPUMemoryTransferOutSyncList::removeMemoryTransfer ( GPUMemoryTransfer &mt )
{
   GPUDevice::copyOutSyncToHost( ( void * ) mt._hostAddress.getTag(), mt._deviceAddress, mt._size );
   ( ( GPUProcessor * ) myThread->runningOn() )->synchronize( mt._hostAddress );
}

void GPUMemoryTransferOutSyncList::clearRequestedMemoryTransfers ()
{
   _lock.acquire();
   for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
         it != _pendingTransfersAsync.end();
         it++ )
   {
      if ( it->_requested ) {
         //_lock.acquire();
         GPUMemoryTransfer mt ( *it );
         it = _pendingTransfersAsync.erase( it );
         //_lock.release();
         removeMemoryTransfer( mt );
      }
   }
   _lock.release();
}

void GPUMemoryTransferOutSyncList::executeMemoryTransfers ()
{
   while ( !_pendingTransfersAsync.empty() ) {
      _lock.acquire();
      GPUMemoryTransfer mt ( *_pendingTransfersAsync.begin() );
      _pendingTransfersAsync.erase( _pendingTransfersAsync.begin() );
      _lock.release();

      removeMemoryTransfer( mt );
   }
}


void GPUMemoryTransferOutAsyncList::removeMemoryTransfer ( GPUMemoryTransfer &mt )
{
#ifndef JBUENO_NO_PINNING
   nanos::ext::GPUProcessor * myPE = ( nanos::ext::GPUProcessor * ) myThread->runningOn();
   void * pinned = myPE->allocateOutputPinnedMemory( mt._size );

   // Even there is only one copy, we must do it asynchronously, as we may be doing something else
   GPUDevice::copyOutAsyncToBuffer( pinned, mt._deviceAddress, mt._size );
   GPUDevice::copyOutAsyncWait();
   GPUDevice::copyOutAsyncToHost( ( void * ) mt._hostAddress.getTag(), pinned, mt._size );
#else
   GPUDevice::copyOutAsyncToBuffer( ( void * ) mt._hostAddress.getTag(), mt._deviceAddress, mt._size );
   GPUDevice::copyOutAsyncWait();
#endif

   ( ( GPUProcessor * ) myThread->runningOn() )->synchronize( mt._hostAddress );
}

void GPUMemoryTransferOutAsyncList::removeMemoryTransfer ( CopyDescriptor &hostAddress )
{
   for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
         it != _pendingTransfersAsync.end();
         it++ )
   {
      if ( it->_hostAddress.getTag() == hostAddress.getTag() ) {
         _lock.acquire();
         GPUMemoryTransfer mt ( *it );
         it = _pendingTransfersAsync.erase( it );
         _lock.release();
         removeMemoryTransfer( mt );
      }
   }
}

void GPUMemoryTransferOutAsyncList::executeRequestedMemoryTransfers ()
{
   std::list<GPUMemoryTransfer> itemsToRemove;
   _lock.acquire();
   for ( std::list<GPUMemoryTransfer>::iterator it = _pendingTransfersAsync.begin();
         it != _pendingTransfersAsync.end();
         it++ )
   {
      if ( it->_requested ) {
         //_lock.acquire();
         itemsToRemove.push_back(*it);
         it = _pendingTransfersAsync.erase( it );
         //_lock.release();
      }
   }
   _lock.release();
   executeMemoryTransfers( itemsToRemove );
}

void GPUMemoryTransferOutAsyncList::executeMemoryTransfers ( std::list<GPUMemoryTransfer> &pendingTransfersAsync )
{
   if ( !pendingTransfersAsync.empty() ) {

      nanos::ext::GPUProcessor * myPE = ( nanos::ext::GPUProcessor * ) myThread->runningOn();

      // First copy
      std::list<GPUMemoryTransfer>::iterator it1 = pendingTransfersAsync.begin();

      _lock.acquire();
      while( it1 != pendingTransfersAsync.end() && !it1->_requested ) {
         it1++;
      }
      if ( it1 == pendingTransfersAsync.end() ) it1 = pendingTransfersAsync.begin();

      GPUMemoryTransfer mt1 ( *it1 );
      pendingTransfersAsync.erase( it1 );
      _lock.release();

#ifndef JBUENO_NO_PINNING
      void * pinned1 = myPE->allocateOutputPinnedMemory( mt1._size );

      GPUDevice::copyOutAsyncToBuffer( pinned1, mt1._deviceAddress, mt1._size );
#endif

      while ( pendingTransfersAsync.size() > 1) {
#ifndef JBUENO_NO_PINNING
         // First copy
         GPUDevice::copyOutAsyncWait();
#endif

         // Second copy
         // Check if there is another GPUMemoryTransfer requested
         _lock.acquire();
         std::list<GPUMemoryTransfer>::iterator it2 = pendingTransfersAsync.begin();
         while( !it2->_requested && it2 != pendingTransfersAsync.end() ) {
            it2++;
         }
         // If no requested transfer is found, take the first transfer
         if ( it2 == pendingTransfersAsync.end() ) {
            it2 = pendingTransfersAsync.begin();
         }

         GPUMemoryTransfer mt2 ( *it2 );
         pendingTransfersAsync.erase( it2 );
         _lock.release();

#ifndef JBUENO_NO_PINNING
         void * pinned2 = myPE->allocateOutputPinnedMemory( mt2._size );

         GPUDevice::copyOutAsyncToBuffer( pinned2, mt2._deviceAddress, mt2._size );

         // First copy
         GPUDevice::copyOutAsyncToHost( ( void * ) mt1._hostAddress.getTag(), pinned1, mt1._size );
#else
         GPUDevice::copyOutAsyncToBuffer( ( void * ) mt1._hostAddress.getTag(), mt1._deviceAddress, mt1._size );
         GPUDevice::copyOutAsyncWait();
#endif

         // Synchronize first copy
         myPE->synchronize( mt1._hostAddress );

         // Update second copy to be first copy at next iteration
         mt1 = mt2;
#ifndef JBUENO_NO_PINNING
         pinned1 = pinned2;
#endif
      }

#ifndef JBUENO_NO_PINNING
      GPUDevice::copyOutAsyncWait();
      GPUDevice::copyOutAsyncToHost( ( void * ) mt1._hostAddress.getTag(), pinned1, mt1._size );
#else
      GPUDevice::copyOutAsyncToBuffer( ( void * ) mt1._hostAddress.getTag(), mt1._deviceAddress, mt1._size );
      GPUDevice::copyOutAsyncWait();
#endif

      // Synchronize copy
      myPE->synchronize( mt1._hostAddress );

      myPE->freeOutputPinnedMemory();
   }
}


void GPUMemoryTransferInAsyncList::clearMemoryTransfers()
{
   ( ( GPUProcessor * ) myThread->runningOn() )->synchronize( _pendingTransfersAsync );

   _pendingTransfersAsync.clear();
}

void GPUMemoryTransferInAsyncList::removeMemoryTransfer ( GPUMemoryTransfer &mt )
{
#ifndef JBUENO_NO_PINNING
   void *pinned = ( ( nanos::ext::GPUProcessor * ) myThread->runningOn() )->allocateInputPinnedMemory( mt._size );

   GPUDevice::copyInAsyncToBuffer( pinned, ( void * ) mt._hostAddress.getTag(), mt._size );
   GPUDevice::copyInAsyncToDevice( mt._deviceAddress, pinned, mt._size );
#else
   GPUDevice::copyInAsyncToDevice( mt._deviceAddress, (void *)mt._hostAddress.getTag(), mt._size );
#endif
}

void GPUMemoryTransferInAsyncList::executeMemoryTransfers ()
{
   while ( !_requestedTransfers.empty() ) {
      _lock.acquire();
      GPUMemoryTransfer mt ( *_requestedTransfers.begin() );
      _requestedTransfers.erase( _requestedTransfers.begin() );
      _lock.release();

      removeMemoryTransfer( mt );
   }
}
