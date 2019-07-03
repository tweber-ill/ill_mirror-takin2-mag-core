/**
 * calculates TAS angles from rlu
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-jun-19
 * @license see 'LICENSE' file
 */

public class TasCalc
{
    // calculated with scipy, see tascalc.py
    protected static final double E_to_k2 = 0.482596406464;


    public TasCalc()
    {
    }


    // ------------------------------------------------------------------------
    /**
     * crystallographic A matrix converting fractional to lab coordinates
     * see: https://de.wikipedia.org/wiki/Fraktionelle_Koordinaten
     */
    public static double[][] get_A(double[] lattice, double[] angles)
        throws Exception
    {
        double[] cs = new double[]
        {
            Math.cos(angles[0]), 
            Math.cos(angles[1]), 
            Math.cos(angles[2]) 
        };

        double s2 = Math.sin(angles[2]);

        double[][] A = new double[][]
        {
            {
                lattice[0] * 1.,
                0.,
                0.
            },
            {
                lattice[1] * cs[2],
                lattice[1] * s2,
                0.
            },    
            {
                lattice[2] * cs[1],
                lattice[2] * (cs[0]-cs[1]*cs[2]) / s2,
                lattice[2] * (Math.sqrt(1. - Calc.dot(cs, cs) + 2.*cs[0]*cs[1]*cs[2])) / s2
            }
        };

        return A;
    }


    /**
     * crystallographic B matrix converting rlu to 1/A
     * the reciprocal-space basis vectors form the columns of the B matrix
     */
    public static double[][] get_B(double[] lattice, double[] angles)
        throws Exception
    {
        double[][] A = get_A(lattice, angles);
        double[][] B = Calc.mul(2.*Math.PI, Calc.transpose(Calc.inv(A)));
        return B;
    }


    /**
     * UB orientation matrix
     */
    public static double[][] get_UB(double[][] B, double[] orient1_rlu, double[] orient2_rlu, double[] orientup_rlu)
        throws Exception
    {
        double[] orient1_invA = Calc.dot(B, orient1_rlu);
        double[] orient2_invA = Calc.dot(B, orient2_rlu);
        double[] orientup_invA = Calc.dot(B, orientup_rlu);

        orient1_invA = Calc.div(orient1_invA, Calc.norm_2(orient1_invA));
        orient2_invA = Calc.div(orient2_invA, Calc.norm_2(orient2_invA));
        orientup_invA = Calc.div(orientup_invA, Calc.norm_2(orientup_invA));

        final int N = B.length;
        double[][] U_invA = new double[N][N];
        for(int i=0; i<N; ++i)
        {
            U_invA[0][i] = orient1_invA[i];
            U_invA[1][i] = orient2_invA[i];
            U_invA[2][i] = orientup_invA[i];
        }
          
        double[][] UB = Calc.dot(U_invA, B);
        return UB;
    }

    // ------------------------------------------------------------------------



    // ------------------------------------------------------------------------
    /**
     *  mono (or ana) k  ->  A1 angle (or A5)
     */
    public static double get_a1(double k, double d)
    {
        double s = Math.PI/(d*k);
        double a1 = Math.asin(s);
        return a1;
    }


    /**
     * a1 angle (or a5)  ->  mono (or ana) k
     */
    public static double get_monok(double theta, double d)
    {
        double s = Math.sin(theta);
        double k = Math.PI/(d*s);
        return k;
    }
    // ------------------------------------------------------------------------


    // ------------------------------------------------------------------------
    /**
     * scattering triangle
     * get scattering angle a4
     */
    public static double get_a4(double ki, double kf, double Q)
    {
        double c = (ki*ki + kf*kf - Q*Q) / (2.*ki*kf);
        return Math.acos(c);
    }
    

    /**
     * scattering triangle
     * get |Q| from ki, kf and a4
     */
    public static double get_Q(double ki, double kf, double a4)
    {
        double c = Math.cos(a4);
        return Math.sqrt(ki*ki + kf*kf - c*(2.*ki*kf));
    }


    /**
     * scattering triangle
     * get angle enclosed by ki and Q
     */
    public static double get_psi(double ki, double kf, double Q, double sense/*=1.*/)
    {
        double c = (ki*ki + Q*Q - kf*kf) / (2.*ki*Q);
        return sense * Math.acos(c);
    }
    

    /**
     * scattering triangle
     * get ki from kf and energy transfer E
     */
    public static double get_ki(double kf, double E)
    {
        return Math.sqrt(kf*kf + E_to_k2*E);
    }


    /**
     * scattering triangle
     * get kf from ki and energy transfer E
     */
    public static double get_kf(double ki, double E)
    {
        return Math.sqrt(ki*ki - E_to_k2*E);
    }


    /**
     * scattering triangle
     * get energy transfer E from ki and kf
     */
    public static double get_E(double ki, double kf)
    {
        return (ki*ki - kf*kf) / E_to_k2;
    }
    // ------------------------------------------------------------------------
}
