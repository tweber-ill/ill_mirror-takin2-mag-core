/**
 * unit test for tas calculator
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-jun-19
 * @license see 'LICENSE' file
 * 
 * javac -cp .:/Users/tw/tmp/junit-4.13-beta-3.jar Calc.java TasCalc.java TestTasCalc.java
 * java -cp .:/Users/tw/tmp/junit-4.13-beta-3.jar:/Users/tw/tmp/hamcrest-core-1.3.jar org.junit.runner.JUnitCore TestTasCalc
 */

import junit.framework.TestCase;
import org.junit.Assert;


public class TestTasCalc extends TestCase
{
    public void setUp()
    {   
    }


    public void testDot()
    {
        try
        {
            double[] x = new double[]{1., 2., 3.};
            double[] y = new double[]{4., 5., 6.};

            double d = Calc.dot(x, y);
            assertEquals(32., d, 1e-6);
        }
        catch(Exception ex)
        {
            System.err.println(ex.toString());
        }
    }


    public void testCross()
    {
        try
        {
            double[] x = new double[]{1., 2., 3.};
            double[] y = new double[]{9., -8., 7.};

            double[] r = Calc.cross(x, y);
            double[] res = new double[]{38., 20., -26.};
            Assert.assertArrayEquals(res, r, 1e-6);
        }
        catch(Exception ex)
        {
            System.err.println(ex.toString());
        }
    }


    public void testDet()
        throws Exception
    {
        double[][] M2 = new double[][]
        {
            {1., 2.},
            {3., 4.}
        };

        double[][] M3 = new double[][]
        {
            {1., -2., 3.},
            {4., 5., -6.},
            {7., 8., 9.}
        };

        double d2 = Calc.det(M2);
        assertEquals("Wrong determinant!", -2., d2, 1e-6);
 
        double d3 = Calc.det(M3);
        assertEquals("Wrong determinant!", 240., d3, 1e-6);
    }


    public void testInv()
        throws Exception
    {
        double[][] M2 = new double[][]
        {
            {1., 2.},
            {3., 4.}
        };

        double[][] M3 = new double[][]
        {
            {1., -2., 3.},
            {4., 5., -6.},
            {7., 8., 9.}
        };

        double[][] _I2 = new double[][]
        {
            {-2., 1.},
            {1.5, -0.5}
        };

        double[][] _I3 = new double[][]
        {
            {0.3875, 0.175, -0.0125},
            {-0.325, -0.05, 0.075},
            {-0.0125, -0.09166666, 0.05416666}
        };

        double[][] I2 = Calc.inv(M2);
        for(int i=0; i<I2.length; ++i)
            Assert.assertArrayEquals("Wrong inverse!", _I2[i], I2[i], 1e-6);
 
        double[][] I3 = Calc.inv(M3);
        for(int i=0; i<I3.length; ++i)
            Assert.assertArrayEquals("Wrong inverse!", _I3[i], I3[i], 1e-6);
    }


    public void testMono()
        throws Exception
    {
        double d = 3.437;
        double theta = Math.toRadians(78.15/2.);
        double k = TasCalc.get_monok(theta, d);
        assertEquals("Wrong mono k!", 1.45, k, 1e-4);

        double a1 = TasCalc.get_a1(k, d);
        assertEquals("Wrong a1 angle!", theta, a1, 1e-4);
    }


    public void testQ()
        throws Exception
    {
        double ki = 1.444;
        double kf = 1.45;
        double a4 = Math.toRadians(84.74);
        double Q = 1.951;
        double Q_2 = TasCalc.get_Q(ki, kf, a4);
        assertEquals("Wrong Q!", Q, Q, 1e-3);

        double a4_2 = TasCalc.get_a4(ki, kf, Q);
        assertEquals("Wrong a4 angle!", a4, a4_2, 1e-3);

        double E = -0.036;
        double ki_2 = TasCalc.get_ki(kf, E);
        assertEquals("Wrong ki!", ki, ki_2, 1e-3);

        double E_2 = TasCalc.get_E(ki, kf);
        assertEquals("Wrong E!", E, E_2, 1e-3);
    }


    public void testAngles()
        throws Exception
    {
        double kf = 2.662;
        double E = 2.;
        double ki = TasCalc.get_ki(kf, E);
        double[] Q_rlu = new double[]{1., 2., 2.};
        double[] orient_rlu = new double[]{1., 0., 0.};
        double[] orient_up_rlu = new double[]{-1./3., -2./3., 2./3.};

        double[] lattice = new double[]{5., 5., 5.};
        double[] angles = new double[]{90./180.*Math.PI, 90./180.*Math.PI, 60./180.*Math.PI};

        double[][] B = TasCalc.get_B(lattice, angles);
        double[] a3a4 = TasCalc.get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B, 1.);

        assertEquals("Wrong distance to scattering plane!", 0., a3a4[2], 1e-3);
        assertEquals("Wrong a4!", 80.457, a3a4[1]/Math.PI*180., 1e-3);
        assertEquals("Wrong a3!", 42.389, a3a4[0]/Math.PI*180., 1e-3);


        double[][] metric = Calc.get_metric(B);
        double Qlen = Math.sqrt(Calc.dot(Q_rlu, Q_rlu, metric));
        double[] Qhkl = TasCalc.get_hkl(ki, kf, a3a4[0], Qlen, orient_rlu, orient_up_rlu, B, 1.);
        Assert.assertArrayEquals("Wrong Q position!", new double[]{1., 2., 2.}, Qhkl, 1e-4);
    }
}
