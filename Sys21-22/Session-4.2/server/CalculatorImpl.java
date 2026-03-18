import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;

public class CalculatorImpl extends UnicastRemoteObject implements Calculator {

    public CalculatorImpl() throws RemoteException {
        super();
    }

    public int add(int o1, int o2) throws RemoteException {
        return (o1 + o2);
    }

    public int subtract(int o1, int o2) throws RemoteException {
        return (o1 - o2);
    }
}