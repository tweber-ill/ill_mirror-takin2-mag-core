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
     * r_i = x_i + y_i
     */
    public static double[] add(double[] x, double[] y)
        throws Exception
    {
        final int dim = x.length;
        if(dim != y.length)
            throw new Exception("Vector sizes are not compatible.");

        double[] r = new double[dim];
        for(int i=0; i<dim; ++i)
            r[i] = x[i] + y[i];

        return r;
    }


    /**
     * r_i = x_i - y_i
     */
    public static double[] sub(double[] x, double[] y)
        throws Exception
    {
        final int dim = x.length;
        if(dim != y.length)
            throw new Exception("Vector sizes are not compatible.");

        double[] r = new double[dim];
        for(int i=0; i<dim; ++i)
            r[i] = x[i] - y[i];

        return r;
    }


    /**
     * r_i = x_i * d
     */
    public static double[] mul(double[] x, double d)
        throws Exception
    {
        final int dim = x.length;

        double[] r = new double[dim];
        for(int i=0; i<dim; ++i)
            r[i] = x[i] * d;

        return r;
    }


    /**
     * r_i = d * x_i
     */
    public static double[] mul(double d, double[] x)
        throws Exception
    {
        return mul(x, d);
    }


    /**
     * r_i = x_i / d
     */
    public static double[] div(double[] x, double d)
        throws Exception
    {
        return mul(x, 1./d);
    }


    /**
     * R_ij = A_ij * d
     */
    public static double[][] mul(double[][] A, double d)
        throws Exception
    {
        final int dim1 = A.length;
        final int dim2 = A[0].length;

        double[][] R = new double[dim1][dim2];
        for(int i=0; i<dim1; ++i)
            for(int j=0; j<dim2; ++j)
                R[i][j] = A[i][j] * d;

        return R;
    }


    /**
     * R_ij = d * A_ij
     */
    public static double[][] mul(double d, double[][] A)
        throws Exception
    {
        return mul(A, d);
    }


    /**
     * R_ij = A_ij / d
     */
    public static double[][] div(double[][] A, double d)
        throws Exception
    {
        return mul(A, 1./d);
    }


    /**
     * d = x_i y_i
     */
    public static double dot(double[] x, double[] y)
        throws Exception
    {
        final int dim = x.length;
        if(dim != y.length)
            throw new Exception("Vector sizes are not compatible.");

        double d = 0.;
        for(int i=0; i<dim; ++i)
            d += x[i]*y[i];

        return d;
    }


    /**
     * r_i = eps_ijk x_j y_k
     */
    public static double[] cross(double[] x, double[] y)
        throws Exception
    {
        final int dim = x.length;
        if(dim != y.length)
            throw new Exception("Vector sizes are not compatible.");
        if(dim != 3)
            throw new Exception("Dimension has to be 3.");

        double[] d = new double[]
        {
            x[1]*y[2] - x[2]*y[1],
            x[2]*y[0] - x[0]*y[2],
            x[0]*y[1] - x[1]*y[0]
        };

        return d;
    }


   /**
     * length of vector x
     */
    public static double norm_2(double[] x)
        throws Exception
    {
        return Math.sqrt(dot(x, x));
    }


    /**
     * d_i = M_ij x_j
     */
    public static double[] dot(double[][] M, double[] x)
        throws Exception
    {
        final int dim1 = M.length;
        final int dim2 = M[0].length;
        if(dim2 != x.length)
            throw new Exception("Matrix and vector sizes are not compatible.");

        double[] d = new double[dim1];
        for(int i=0; i<dim1; ++i)
            for(int j=0; j<dim2; ++j)
                d[i] += M[i][j]*x[j];

        return d;
    }


    /**
     * R_ik = A_ij x B_jk
     */
    public static double[][] dot(double[][] A, double[][] B)
        throws Exception
    {
        final int dim1 = A.length;
        final int dim2 = B[0].length;
        final int diminner = A[0].length;
        if(diminner != B.length)
            throw new Exception("Matrix sizes are not compatible.");

        double[][] R = new double[dim1][dim2];
        for(int i=0; i<dim1; ++i)
            for(int k=0; k<dim2; ++k)
                for(int j=0; j<diminner; ++j)
                    R[i][k] += A[i][j] * B[j][k];

        return R;
    }


    /**
     * R_ij = A_ij + B_ij
     */
    public static double[][] add(double[][] A, double[][] B)
        throws Exception
    {
        final int dim1 = A.length;
        final int dim2 = A[0].length;
        if(dim1 != B.length || dim2 != B[0].length)
            throw new Exception("Matrix sizes are not compatible.");

        double[][] R = new double[dim1][dim2];
        for(int i=0; i<dim1; ++i)
            for(int j=0; j<dim2; ++j)
                R[i][j] = A[i][j] + B[i][j];

        return R;
    }


    /**
     * R_ij = A_ij - B_ij
     */
    public static double[][] sub(double[][] A, double[][] B)
        throws Exception
    {
        final int dim1 = A.length;
        final int dim2 = A[0].length;
        if(dim1 != B.length || dim2 != B[0].length)
            throw new Exception("Matrix sizes are not compatible.");

        double[][] R = new double[dim1][dim2];
        for(int i=0; i<dim1; ++i)
            for(int j=0; j<dim2; ++j)
                R[i][j] = A[i][j] - B[i][j];

        return R;
    }


    /**
     * M_ij -> M_ji
     */
    public static double[][] transpose(double[][] M)
        throws Exception
    {
        final int dim1 = M.length;
        final int dim2 = M[0].length;

        double[][] R = new double[dim2][dim1];
        for(int i=0; i<dim1; ++i)
            for(int j=0; j<dim2; ++j)
                R[j][i] = M[i][j];

        return R;
    }


    /**
     * submatrix
     * @param M matrix
     * @param i row to delete
     * @param j column to delete
     * @return submatrix
     */
    public static double[][] submat(double[][] M, int i, int j)
    {
        final int dim1 = M.length;
        final int dim2 = M[0].length;

        double[][] R = new double[dim1-1][dim2-1];

        int _i2 = 0;
        for(int _i=0; _i<dim1; ++_i)
        {
            if(_i == i)
                continue;

            int _j2 = 0;
            for(int _j=0; _j<dim2; ++_j)
            {
                if(_j == j)
                    continue;
                
                R[_i2][_j2] = M[_i][_j];
                ++_j2;
            }
            ++_i2;
        }

        return R;
    }


    /**
     * determinant
     * @param M matrix
     * @return determinant
     * @throws Exception
     */
    public static double det(double[][] M)
        throws Exception
    {
        final int dim1 = M.length;
        final int dim2 = M[0].length;

        if(dim1 != dim2)
            throw new Exception("Expecting a square matrix.");
        
        if(dim1 <= 0)
            return 0.;
        else if(dim1 == 1)
            return M[0][0];
        else if(dim1 == 2)
            return M[0][0]*M[1][1] - M[0][1]*M[1][0];

        double d = 0.;
        int i = 0;
        for(int j=0; j<dim2; ++j)
        {
            if(Math.abs(M[i][j]) < Double.MIN_VALUE)
                continue;

            double sgn = (((i+j) % 2) == 0) ? 1. : -1.;
            d += sgn * M[i][j] * det(submat(M, i,j));
        }
        
        return d;
    }


    /**
     * inverse
     * @param M matrix
     * @return inverse
     * @throws Exception
     */
    public static double[][] inv(double[][] M)
        throws Exception
    {
        final int dim1 = M.length;
        final int dim2 = M[0].length;

        if(dim1 != dim2)
            throw new Exception("Expecting a square matrix.");
 
        double d = det(M);
        double[][] I = new double[dim2][dim1];

        for(int i=0; i<dim1; ++i)
        {
            for(int j=0; j<dim2; ++j)
            {
                double sgn = ((i+j) % 2) == 0 ? 1. : -1.;
                I[j][i] = sgn * det(submat(M, i,j)) / d;
            }
        }

        return I;
    }


    /**
     * rotates a vector around an axis using Rodrigues' formula
     * see: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
     */
    public static double[] rotate(double[] _axis, double[] vec, double phi)
        throws Exception
    {
        double[] axis = div(_axis, norm_2(_axis));

        double s = Math.sin(phi);
        double c = Math.cos(phi);

        double[] vec1 = mul(vec, c);
        double[] vec2 = mul(mul(axis, dot(vec, axis)), (1.-c));
        double[] vec3 = mul(cross(axis, vec), s);

        return add(add(vec1, vec2), vec3);
    }


    /**
     * dot product in fractional coordinates
     */
    public static double dot(double[] x, double[] y, double[][] metric)
        throws Exception
    {
        return dot(x, dot(metric, y));
    }


    /**
     * angle between peaks in fractional coordinates
     */
    public static double angle(double[] x, double[] y, double[][] metric)
        throws Exception
    {
        double len_x = Math.sqrt(dot(x, x, metric));
        double len_y = Math.sqrt(dot(y, y, metric));

        double c = dot(x, y, metric) / (len_x * len_y);
        return Math.acos(c);
    }


    /**
     * get metric from crystal B matrix
     * basis vectors are in the columns of B, i.e. the second index
     */
    public static double[][] get_metric(double[][] B)
        throws Exception
    {
        return dot(transpose(B), B);
    }
    // ------------------------------------------------------------------------



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
                lattice[2] * (Math.sqrt(1. - dot(cs, cs) + 2.*cs[0]*cs[1]*cs[2])) / s2
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
        double[][] B = mul(2.*Math.PI, transpose(inv(A)));
        return B;
    }


    /**
     * UB orientation matrix
     */
    public static double[][] get_UB(double[][] B, double[] orient1_rlu, double[] orient2_rlu, double[] orientup_rlu)
        throws Exception
    {
        double[] orient1_invA = dot(B, orient1_rlu);
        double[] orient2_invA = dot(B, orient2_rlu);
        double[] orientup_invA = dot(B, orientup_rlu);

        orient1_invA = div(orient1_invA, norm_2(orient1_invA));
        orient2_invA = div(orient2_invA, norm_2(orient2_invA));
        orientup_invA = div(orientup_invA, norm_2(orientup_invA));

        final int N = B.length;
        double[][] U_invA = new double[N][N];
        for(int i=0; i<N; ++i)
        {
            U_invA[0][i] = orient1_invA[i];
            U_invA[1][i] = orient2_invA[i];
            U_invA[2][i] = orientup_invA[i];
        }
          
        double[][] UB = dot(U_invA, B);
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
