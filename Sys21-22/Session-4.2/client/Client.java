import java.rmi.Naming;
import java.rmi.RemoteException;
import java.io.*;

public class Client {
    public static void main(String[] args) {
        int op1, op2, res;

        try {
            Calculator obj = (Calculator) Naming.lookup("CalculatorServer");
            BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
            
            try {
                System.out.print("Operand1: ");
                op1 = Integer.parseInt(in.readLine());

                System.out.print("Operand2: ");
                op2 = Integer.parseInt(in.readLine());

                res = obj.add(op1, op2);
                System.out.println("The result of the add is: " + res);

                res = obj.subtract(op1, op2);
                System.out.println("The result of the subtraction is: " + res);
            } catch (RemoteException e) {
                System.out.println("Remote invocation failed: " + e.getMessage());
            } catch (Exception e) {
                System.out.println("Error reading data: " + e.getMessage());
            }
        } catch (Exception e) {
            System.out.println("Lookup failed: " + e.getMessage());
        }
    }
}
