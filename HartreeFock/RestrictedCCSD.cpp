#include "stdafx.h"
#include "RestrictedCCSD.h"


namespace HartreeFock {

	RestrictedCCSD::RestrictedCCSD(int iterations)
		: RestrictedHartreeFock(iterations), numberOfSpinOrbitals(0), numberOfOccupiedSpinOrbitals(0), m_spinOrbitalBasisIntegrals(nullptr)
	{

	}

	RestrictedCCSD::~RestrictedCCSD()
	{
		delete m_spinOrbitalBasisIntegrals;
	}

	void RestrictedCCSD::Init(Systems::Molecule* molecule)
	{
		RestrictedHartreeFock::Init(molecule);

		numberOfSpinOrbitals = 2 * numberOfOrbitals;
		numberOfOccupiedSpinOrbitals = 2 * nrOccupiedLevels;

		if (m_spinOrbitalBasisIntegrals) delete m_spinOrbitalBasisIntegrals;
		m_spinOrbitalBasisIntegrals = new GaussianIntegrals::CoupledClusterSpinOrbitalsElectronElectronIntegralsRepository(numberOfOrbitals);		
	}

	void RestrictedCCSD::CalculateTaus()
	{
		const int numberOfUnoccupiedSpinOrbitals = numberOfSpinOrbitals - numberOfOccupiedSpinOrbitals;

		tau.resize(numberOfOccupiedSpinOrbitals, numberOfOccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);
		taut.resize(numberOfOccupiedSpinOrbitals, numberOfOccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);

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

						const double term2 = t2(indi, inda) * t2(indj, indb) - t2(indi, indb) * t2(indj, inda);

						tau(indi, indj, inda, indb) = t4(indi, indj, inda, indb) + 0.5 * term2;
						taut(indi, indj, inda, indb) = t4(indi, indj, inda, indb) + term2;

						++indb;
					}

					++inda;
				}

				++indj;
			}

			++indi;
		}

	}

	// formula 3
	void RestrictedCCSD::CalculateFae()
	{
		const int numberOfUnoccupiedSpinOrbitals = numberOfSpinOrbitals - numberOfOccupiedSpinOrbitals;
		Fae.resize(numberOfUnoccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);

		int inda = 0;
		for (int a = 0; a < numberOfSpinOrbitals; ++a)
		{
			const int orba = a / 2;
			if (orba < occupied.size() && occupied[orba]) continue; // only unoccupied

			int inde = 0;
			for (int e = 0; e < numberOfSpinOrbitals; ++e)
			{
				const int orbe = e / 2;
				if (orbe < occupied.size() && occupied[orbe]) continue; // only unoccupied
	
				double sum1 = 0;
				double sum2 = 0;
				double sum3 = 0;
			
				int indm = 0;
				for (int m = 0; m < numberOfSpinOrbitals; ++m)
				{
					const int orbm = m / 2;
					if (orbm >= occupied.size() || !occupied[orbm]) continue; // only occupied

					sum1 += f(m, e) * t2(indm, inda);

					int indf = 0;
					for (int f = 0; f < numberOfSpinOrbitals; ++f)
					{
						const int orbf = f / 2;
						if (orbf < occupied.size() && occupied[orbf]) continue; // only unoccupied
					
						sum2 += t2(indm, indf) * (*m_spinOrbitalBasisIntegrals)(m, a, f, e);

						int indn = 0;
						for (int n = 0; n < numberOfSpinOrbitals; ++n)
						{
							const int orbn = n / 2;
							if (orbn >= occupied.size() || !occupied[orbn]) continue; // only occupied
						
							sum3 += taut(indm, indn, inda, indf) * (*m_spinOrbitalBasisIntegrals)(m, n, e, f);

							++indn;
						}					
						++indf;
					}
					++indm;
				}
								
				Fae(inda, inde) = oneminusdelta(inda, inde) * f(a, e) - 0.5 * (sum1 + sum3) + sum2;
				
				++inde;
			}

			++inda;
		}
	}

	// formula 4
	void RestrictedCCSD::CalculateFmi()
	{
		Fmi.resize(numberOfOccupiedSpinOrbitals, numberOfOccupiedSpinOrbitals);

		int indm = 0;
		for (int m = 0; m < numberOfSpinOrbitals; ++m)
		{
			const int orbm = m / 2;
			if (orbm >= occupied.size() || !occupied[orbm]) continue; // only occupied
		
			int indi = 0;
			for (int i = 0; i < numberOfSpinOrbitals; ++i)
			{
				const int hi = i / 2;
				if (hi >= occupied.size() || !occupied[hi]) continue; // only occupied
			
				double sum1 = 0;
				double sum2 = 0;
				double sum3 = 0;

				int inde = 0;
				for (int e = 0; e < numberOfSpinOrbitals; ++e)
				{
					const int orbe = e / 2;
					if (orbe < occupied.size() && occupied[orbe]) continue; // only unoccupied

					sum1 += f(m, e) * t2(indi, inde);

					int indn = 0;
					for (int n = 0; n < numberOfSpinOrbitals; ++n)
					{
						const int orbn = n / 2;
						if (orbn >= occupied.size() || !occupied[orbn]) continue; // only occupied

						sum2 += t2(indn, inde) * (*m_spinOrbitalBasisIntegrals)(m, n, i, e);

						int indf = 0;
						for (int f = 0; f < numberOfSpinOrbitals; ++f)
						{
							const int orbf = f / 2;
							if (orbf < occupied.size() && occupied[orbf]) continue; // only unoccupied

							sum3 += taut(indi, indn, inde, indf) * (*m_spinOrbitalBasisIntegrals)(m, n, e, f);

							++indf;
						}
						++indn;
					}

					++inde;
				}


				Fmi(indm, indi) = oneminusdelta(indm, indi) * f(m, i) + 0.5 * (sum1 + sum3) + sum2;

				++indi;
			}

			++indm;
		}
	}


	// formula 5
	void RestrictedCCSD::CalculateFme()
	{
		const int numberOfUnoccupiedSpinOrbitals = numberOfSpinOrbitals - numberOfOccupiedSpinOrbitals;

		Fme.resize(numberOfOccupiedSpinOrbitals, numberOfUnoccupiedSpinOrbitals);

		int indm = 0;
		for (int m = 0; m < numberOfSpinOrbitals; ++m)
		{
			const int orbm = m / 2;
			if (orbm >= occupied.size() || !occupied[orbm]) continue; // only occupied

			int inde = 0;
			for (int e = 0; e < numberOfSpinOrbitals; ++e)
			{
				const int orbe = e / 2;
				if (orbe < occupied.size() && occupied[orbe]) continue; // only unoccupied

				double nfSum = 0;

				int indn = 0;
				for (int n = 0; n < numberOfSpinOrbitals; ++n)
				{
					const int orbn = n / 2;
					if (orbn >= occupied.size() || !occupied[orbn]) continue; // only occupied

					int indf = 0;
					for (int f = 0; f < numberOfSpinOrbitals; ++f)
					{
						const int orbf = f / 2;
						if (orbf < occupied.size() && occupied[orbf]) continue; // only unoccupied
					
						nfSum += t2(indn, indf) * (*m_spinOrbitalBasisIntegrals)(m, n, e, f);

						++indf;
					}
				
					++indn;
				}


				Fme(indm, inde) = f(m, e) + nfSum;

				++inde;
			}

			++indm;
		}
	}





	void RestrictedCCSD::CalculateIntermediates()
	{
		// arrays
		CalculateTaus();

		CalculateFae();
		CalculateFmi();
		CalculateFme();

		// 4 indexes tensors


	}


}
