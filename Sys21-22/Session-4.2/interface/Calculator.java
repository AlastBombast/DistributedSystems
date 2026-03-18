import java.rmi.Remote;

public interface Calculator extends Remote {
    public int add(int o1, int o2) throws java.rmi.RemoteException;

    public int subtract(int o1, int o2) throws java.rmi.RemoteException;
}
