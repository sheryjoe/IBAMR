// Filename: IBRodForceSpecFactory.h
// Created on 23 Jun 2010 by Boyce Griffith
//
// Copyright (c) 2002-2010 Boyce Griffith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef included_IBRodForceSpecFactory
#define included_IBRodForceSpecFactory

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/Stashable.h>
#include <ibtk/StashableFactory.h>

// SAMRAI INCLUDES
#include <IntVector.h>
#include <tbox/AbstractStream.h>
#include <tbox/Pointer.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBRodForceSpecFactory is a factory class to rebuild
 * IBRodForceSpec objects from SAMRAI::tbox::AbstractStream data streams.
 */
class IBRodForceSpecFactory
    : public IBTK::StashableFactory
{
public:
    /*!
     * \brief Default constructor.
     */
    IBRodForceSpecFactory();

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBRodForceSpecFactory();

    /*!
     * \brief Return the unique identifier used to specify the
     * IBTK::StashableFactory object used by the IBTK::StashableManager to
     * extract Stashable objects from data streams.
     */
    virtual int
    getStashableID() const;

    /*!
     * \brief Set the unique identifier used to specify the
     * IBTK::StashableFactory object used by the IBTK::StashableManager to
     * extract Stashable objects from data streams.
     */
    virtual void
    setStashableID(
        const int stashable_id);

    /*!
     * \brief Build a IBTK::Stashable object by unpacking data from the input
     * stream.
     */
    virtual SAMRAI::tbox::Pointer<IBTK::Stashable>
    unpackStream(
        SAMRAI::tbox::AbstractStream& stream,
        const SAMRAI::hier::IntVector<NDIM>& offset);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBRodForceSpecFactory(
        const IBRodForceSpecFactory& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBRodForceSpecFactory&
    operator=(
        const IBRodForceSpecFactory& that);

    /*
     * The stashable ID for this object type.
     */
    static int s_stashable_id;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBRodForceSpecFactory.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBRodForceSpecFactory
