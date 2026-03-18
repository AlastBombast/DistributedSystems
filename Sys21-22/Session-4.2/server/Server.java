import java.rmi.Naming;

public class Server {
    public static void main(String[] args) {
        try {

            CalculatorImpl obj = new CalculatorImpl();

            Naming.rebind("CalculatorServer", obj);
            System.out.println("Registered CalculatorServer");

        } catch (Exception e) {
            System.out.println("Error in CalculatorImpl: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
