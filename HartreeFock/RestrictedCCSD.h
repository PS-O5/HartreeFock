#pragma once
#include "RestrictedHartreeFock.h"

#include "CoupledClusterSpinOrbitalsElectronElectronIntegralsRepository.h"

#include <unsupported/Eigen/CXX11/Tensor>

#include <eigen/eigen>

// implementation guided by this tutorial: https://github.com/CrawfordGroup/ProgrammingProjects/tree/master/Project%2305

namespace HartreeFock {

    class RestrictedCCSD :
        public RestrictedHartreeFock
    {
    public:
        RestrictedCCSD(int iterations = 3000);
        virtual ~RestrictedCCSD();

        virtual void Init(Systems::Molecule* molecule) override;

        void InitCC()
        {
            m_spinOrbitalBasisIntegrals->Compute(integralsRepository, C);
            InitialGuessClusterAmplitudes();
            f = getSpinOrbitalFockMatrix();
        }

    private:

        // this should correspond to Step #1: Preparing the Spin-Orbital Basis Integrals
        // implemented in a hurry, needs checking

        inline Eigen::MatrixXd getSpinOrbitalFockMatrix()
        {
            Eigen::MatrixXd spinOrbitalFockMatrix(numberOfSpinOrbitals, numberOfSpinOrbitals);

            Eigen::MatrixXd FockMatrixMO = C.transpose() * h * C;

            for (int p = 0; p < numberOfSpinOrbitals; ++p)
            {
                const int hp = p / 2;

                for (int q = 0; q < numberOfSpinOrbitals; ++q)
                {
                    spinOrbitalFockMatrix(p, q) = (p % 2 == q % 2) * FockMatrixMO(hp, q / 2);
                    for (int m = 0; m < numberOfSpinOrbitals; ++m)
                    {
                        const int orb = m / 2;
                        if (orb >= occupied.size() || !occupied[orb]) continue; // only occupied

                        spinOrbitalFockMatrix(p, q) += (*m_spinOrbitalBasisIntegrals)(p, m, q, m);
                    }
                }
            }

            return spinOrbitalFockMatrix;
        }


        // Step #2: Build the Initial-Guess Cluster Amplitudes
        void InitialGuessClusterAmplitudes()
        {
            const int numberOfUnoccupiedSpinOrbitals = numberOfSpinOrbitals - numberOfOccupiedSpinOrbitals;

            t2 = Eigen::MatrixXd::Zero(numberOfOccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);

            t4.resize(numberOfOccupiedSpinOrbitals, numberOfOccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);

            int indi = 0;
            for (int i = 0; i < numberOfSpinOrbitals; ++i)
            {
                const int hi = i / 2;
                if (hi >= occupied.size() || !occupied[hi]) continue; // only occupied

                int indj = 0;
                for (int j = 0; j < numberOfSpinOrbitals; ++j)
                {
                    const int hj = j / 2;
                    if (hj >= occupied.size() || !occupied[hj]) continue; // only occupied

                    int inda = 0;
                    for (int a = 0; a < numberOfSpinOrbitals; ++a)
                    {
                        const int ha = a / 2;
                        if (ha < occupied.size() && occupied[ha]) continue; // only unoccupied

                        int indb = 0;
                        for (int b = 0; b < numberOfSpinOrbitals; ++b)
                        {
                            const int hb = b / 2;
                            if (hb < occupied.size() && occupied[hb]) continue; // only unoccupied

                            t4(indi, indj, inda, indb) = (*m_spinOrbitalBasisIntegrals)(i, j, a, b) / (eigenvals(hi) + eigenvals(hj) - eigenvals(ha) - eigenvals(hb));

                            ++indb;
                        }

                        ++inda;
                    }
                
                    ++indj;
                }

                ++indi;
            }
        }


    public:

        // just for checking against the MP2 energy
        double MP2EnergyFromt4() const
        {
            double result = 0;

            int indi = 0;
            for (int i = 0; i < numberOfSpinOrbitals; ++i)
            {
                const int orbi = i / 2;
                if (orbi >= occupied.size() || !occupied[orbi]) continue; // only occupied

                int indj = 0;
                for (int j = 0; j < numberOfSpinOrbitals; ++j)
                {
                    const int orbj = j / 2;
                    if (orbj >= occupied.size() || !occupied[orbj]) continue; // only occupied

                    int inda = 0;
                    for (int a = 0; a < numberOfSpinOrbitals; ++a)
                    {
                        const int orba = a / 2;
                        if (orba < occupied.size() && occupied[orba]) continue; // only unoccupied

                        int indb = 0;
                        for (int b = 0; b < numberOfSpinOrbitals; ++b)
                        {
                            const int orbb = b / 2;
                            if (orbb < occupied.size() && occupied[orbb]) continue;  // only unoccupied

                            result += (*m_spinOrbitalBasisIntegrals)(i, j, a, b) * t4(indi, indj, inda, indb);

                            ++indb;
                        }

                        ++inda;
                    }

                    ++indj;
                }

                ++indi;
            }


            return 0.25 * result;
        }
        

    private:

        int numberOfSpinOrbitals;
        int numberOfOccupiedSpinOrbitals;

        Eigen::MatrixXd f;


        // TODO: lower the memory usage for t2 and t4, need only elements between occupied and not occupied?

        Eigen::MatrixXd t2;
        
        // they should be better implemented than my own implementation
        Eigen::Tensor<double, 4> t4;
        
        GaussianIntegrals::CoupledClusterSpinOrbitalsElectronElectronIntegralsRepository* m_spinOrbitalBasisIntegrals;
    };

}

