/*
 * PDBLoader.h
 *
 * Copyright (C) 2010 by University of Stuttgart (VISUS).
 * All rights reserved.
 */

#ifndef MMPROTEINPLUGIN_PDBLOADER_H_INCLUDED
#define MMPROTEINPLUGIN_PDBLOADER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "Module.h"
#include "param/ParamSlot.h"
#include "CalleeSlot.h"
#include "vislib/Array.h"
#include "vislib/Vector.h"
#include "vislib/Cuboid.h"
#include "CallProteinData.h"
#include "MolecularDataCall.h"
#include "Stride.h"

namespace megamol {
namespace protein {

    /**
     * Data source for PDB files
     */

    class PDBLoader : public megamol::core::Module
    {
    public:
        
        /** Ctor */
        PDBLoader(void);

        /** Dtor */
        virtual ~PDBLoader(void);

        /**
         * Answer the name of this module.
         *
         * @return The name of this module.
         */
        static const char *ClassName(void)  {
            return "PDBLoader";
        }

        /**
         * Answer a human readable description of this module.
         *
         * @return A human readable description of this module.
         */
        static const char *Description(void) {
            return "Offers protein data.";
        }

        /**
         * Answers whether this module is available on the current system.
         *
         * @return 'true' if the module is available, 'false' otherwise.
         */
        static bool IsAvailable(void) {
            return true;
        }

    protected:

        /**
         * Implementation of 'Create'.
         *
         * @return 'true' on success, 'false' otherwise.
         */
        virtual bool create(void);

        /**
         * Call callback to get the data
         *
         * @param c The calling call
         *
         * @return True on success
         */
        bool getData( core::Call& call);

        /**
         * Call callback to get the extent of the data
         *
         * @param c The calling call
         *
         * @return True on success
         */
        bool getExtent( core::Call& call);

        /**
         * Implementation of 'Release'.
         */
        virtual void release(void);

    private:

        /**
         * Storage of frame data
         */
        class Frame {
        public:

            /** Ctor */
            Frame(void);

            /** Dtor */
            virtual ~Frame(void);

            /**
             * Test for equality
             *
             * @param rhs The right hand side operand
             *
             * @return true if this and rhs are equal
             */
            bool operator==(const Frame& rhs);

            /**
             * Set the atom count.
             *
             * @param atomCnt The atom count
             */
            inline void SetAtomCount( unsigned int atomCnt) { 
                this->atomCount = atomCnt;
                this->atomPosition.SetCount( atomCnt*3);
                this->bfactor.SetCount( atomCnt); 
                this->charge.SetCount( atomCnt); 
                this->occupancy.SetCount( atomCnt); 
            }

            /**
             * Get the atom count.
             *
             * @return The atom count.
             */
            inline unsigned int AtomCount() const { return this->atomCount; }

            /**
             * Assign a position to the array of positions.
             */
            bool SetAtomPosition( unsigned int idx, float x, float y, float z);

            /**
             * Assign a bfactor to the array of bfactors.
             */
            bool SetAtomBFactor( unsigned int idx, float val);

            /**
             * Assign a charge to the array of charges.
             */
            bool SetAtomCharge( unsigned int idx, float val);

            /**
             * Assign a occupancy to the array of occupancies.
             */
            bool SetAtomOccupancy( unsigned int idx, float val);

            /**
             * Set the b-factor range.
             *
             * @param min    The minimum b-factor.
             * @param max    The maximum b-factor.
             */
            void SetBFactorRange( float min, float max) {
                this->minBFactor = min; this->maxBFactor = max; }

            /**
             * Set the minimum b-factor.
             *
             * @param min    The minimum b-factor.
             */
            void SetMinBFactor( float min) { this->minBFactor = min; }

            /**
             * Set the maximum b-factor.
             *
             * @param max    The maximum b-factor.
             */
            void SetMaxBFactor( float max) { this->maxBFactor = max; }

            /**
             * Set the charge range.
             *
             * @param min    The minimum charge.
             * @param max    The maximum charge.
             */
            void SetChargeRange( float min, float max) {
                this->minCharge = min; this->maxCharge = max; }

            /**
             * Set the minimum charge.
             *
             * @param min    The minimum charge.
             */
            void SetMinCharge( float min) { this->minCharge = min; }

            /**
             * Set the maximum charge.
             *
             * @param max    The maximum charge.
             */
            void SetMaxCharge( float max) { this->maxCharge = max; }

            /**
             * Set the occupancy range.
             *
             * @param min    The minimum occupancy.
             * @param max    The maximum occupancy.
             */
            void SetOccupancyRange( float min, float max) {
                this->minOccupancy = min; this->maxOccupancy = max; }

            /**
             * Set the minimum occupancy.
             *
             * @param min    The minimum occupancy.
             */
            void SetMinOccupancy( float min) { this->minOccupancy = min; }

            /**
             * Set the maximum occupancy.
             *
             * @param max    The maximum occupancy.
             */
            void SetMaxOccupancy( float max) { this->maxOccupancy = max; }

            /**
             * Get a reference to the array of atom positions.
             *
             * @return The atom position array.
             */
            const float* AtomPositions() { return this->atomPosition.PeekElements(); }

            /**
             * Get a reference to the array of atom b-factors.
             *
             * @return The atom b-factor array.
             */
            const float* AtomBFactor() { return this->bfactor.PeekElements(); }

            /**
             * Get a reference to the array of atom charges.
             *
             * @return The atom charge array.
             */
            const float* AtomCharge() { return this->charge.PeekElements(); }

            /**
             * Get a reference to the array of atom occupancies.
             *
             * @return The atom occupancy array.
             */
            const float* AtomOccupancy() { return this->occupancy.PeekElements(); }

            /**
             * Get the maximum b-factor of this frame.
             *
             * @return The maximum b-factor.
             */
            float MaxBFactor() const { return this->maxBFactor; }

            /**
             * Get the minimum b-factor of this frame.
             *
             * @return The minimum b-factor.
             */
            float MinBFactor() const { return this->minBFactor; }

            /**
             * Get the maximum b-factor of this frame.
             *
             * @return The maximum b-factor.
             */
            float MaxCharge() const { return this->maxCharge; }

            /**
             * Get the minimum charge of this frame.
             *
             * @return The minimum charge.
             */
            float MinCharge() const { return this->minCharge; }

            /**
             * Get the maximum occupancy of this frame.
             *
             * @return The maximum occupancy.
             */
            float MaxOccupancy() const { return this->maxOccupancy; }

            /**
             * Get the minimum occupancy of this frame.
             *
             * @return The minimum occupancy.
             */
            float MinOccupancy() const { return this->minOccupancy; }

        private:
            /** The atom count */
            unsigned int atomCount;

            /** The atom positions */
            vislib::Array<float> atomPosition;

            /** The atom b-factors */
            vislib::Array<float> bfactor;

            /** The atom charges */
            vislib::Array<float> charge;

            /** The atom occupancy */
            vislib::Array<float> occupancy;

            /** The maximum b-factor */
            float maxBFactor;
            /** The minimum b-factor */
            float minBFactor;

            /** The maximum carge */
            float maxCharge;
            /** The minimum charge */
            float minCharge;

            /** The maximum occupancy */
            float maxOccupancy;
            /** The minimum occupancy */
            float minOccupancy;

        };

        /**
         * Loads a PDB file.
         *
         * @param filename The path to the file to load.
         */
        void loadFile( const vislib::TString& filename);

        /**
         * Parse one atom entry.
         *
         * @param atomEntry The atom entry string.
         * @param atom      The number of the current atom.
         * @param frame     The number of the current frame.
         */
        void parseAtomEntry( vislib::StringA &atomEntry, unsigned int atom, unsigned int frame);

        /**
         * Get the radius of the element.
         *
         * @param name The name of the atom type.
         * @return The radius of the element in Angstrom.
         */
        float getElementRadius( vislib::StringA name);

        /**
         * Get the color of the element.
         *
         * @param name The name of the atom type.
         * @return The color of the element.
         */
        vislib::math::Vector<unsigned char, 3> getElementColor( vislib::StringA name);

        /**
         * Parse one atom entry and set the position of the current atom entry 
         * to the frame.
         *
         * @param atomEntry The atom entry string.
         * @param atom      The number of the current atom.
         * @param frame     The number of the current frame.
         */
        void setAtomPositionToFrame( vislib::StringA &atomEntry, 
            unsigned int atom, unsigned int frame);

        /**
         * Search for connections in the given residue and add them to the
         * global connection array.
         *
         * @param resIdx The index of the residue.
         * @param resIdx The index of the reference frame.
         */
        void MakeResidueConnections( unsigned int resIdx, unsigned int frame);

        /**
         * Search for connections between two residues.
         *
         * @param resIdx0   The index of the first residue.
         * @param resIdx1   The index of the second residue.
         * @param resIdx    The index of the reference frame.
         *
         * @return 'true' if connections were found, 'false' otherwise.
         */
        bool MakeResidueConnections( unsigned int resIdx0, unsigned int resIdx1, unsigned int frame);

        /**
         * Check if the residue is an amino acid.
         *
         * @return 'true' if resName specifies an amino acid, 'false' otherwise.
         */
        bool IsAminoAcid( vislib::StringA resName );

        // -------------------- variables --------------------

        /** The file name slot */
        core::param::ParamSlot filenameSlot;
        /** The data callee slot */
        core::CalleeSlot dataOutSlot;

        /** The maximum frame slot */
        core::param::ParamSlot maxFramesSlot;
        /** The STRIDE usage flag slot */
        core::param::ParamSlot strideFlagSlot;

        /** The data */
        vislib::Array<Frame> data;

        /** The bounding box */
        vislib::math::Cuboid<float> bbox;

        /** The data hash */
        SIZE_T datahash;

        /** Stores for each atom the index of its type */
        vislib::Array<unsigned int> atomTypeIdx;

        /** The array of atom types */
        vislib::Array<MolecularDataCall::AtomType> atomType;

        /** The array of residues */
        vislib::Array<MolecularDataCall::Residue*> residue;

        /** The array of residue type names */
        vislib::Array<vislib::StringA> residueTypeName;

        /** The array of molecules */
        vislib::Array<MolecularDataCall::Molecule> molecule;

        /** The array of chains */
        vislib::Array<MolecularDataCall::Chain> chain;

        /**
         * Stores the connectivity information (i.e. subsequent pairs of atom 
         * indices)
         */
        vislib::Array<unsigned int> connectivity;

        /** Stores the current residue sequence number while loading */
        unsigned int resSeq;

        /** Stores the current molecule count while loading */
        unsigned int molIdx;

        /** Stride secondary structure computation */
        Stride *stride;
        /** Flag wether secondary structure is available */
        bool secStructAvailable;

        // Temporary variables for molecular chains
        vislib::Array<unsigned int> chainFirstRes;
        vislib::Array<unsigned int> chainResCount;
        char chainId;

    };


} /* end namespace protein */
} /* end namespace megamol */

#endif // MMPROTEINPLUGIN_PDBLOADER_H_INCLUDED
