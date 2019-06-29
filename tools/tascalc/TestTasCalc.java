/**
 * unit test for tas calculator
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-jun-19
 * @license see 'LICENSE' file
 * 
 * javac -cp .:/Users/tw/tmp/junit-4.13-beta-3.jar TasCalc.java TestTasCalc.java
 * java -cp .:/Users/tw/tmp/junit-4.13-beta-3.jar:/Users/tw/tmp/hamcrest-core-1.3.jar org.junit.runner.JUnitCore TestTasCalc
 */

import junit.framework.TestCase;
import org.junit.Assert;


public class TestTasCalc extends TestCase
{
    private TasCalc m_tas;


    public void setUp()
    {   
        m_tas = new TasCalc();
    }


    public void testDot()
    {
        try
        {
            double[] x = new double[]{1., 2., 3.};
            double[] y = new double[]{4., 5., 6.};

            double d = TasCalc.dot(x, y);
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

            double[] r = TasCalc.cross(x, y);
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

        double d2 = TasCalc.det(M2);
        assertEquals("Wrong determinant!", -2., d2, 1e-6);
 
        double d3 = TasCalc.det(M3);
        assertEquals("Wrong determinant!", 240., d3, 1e-6);
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
}
