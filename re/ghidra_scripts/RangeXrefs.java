//List every reference into a byte range, grouped by offset from the range base.
//Args: <outFile> <baseHex> <lenHex>
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;

public class RangeXrefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        Address base = toAddr(a[1]);
        long len = Long.parseLong(a[2].replace("0x",""), 16);
        ReferenceManager rm = currentProgram.getReferenceManager();
        w.println("base=" + base + " len=0x" + Long.toHexString(len));
        for (long off = 0; off < len; off++) {
            Address ad = base.add(off);
            ReferenceIterator ri = rm.getReferencesTo(ad);
            List<String> from = new ArrayList<String>();
            while (ri.hasNext()) {
                Reference r = ri.next();
                Function f = getFunctionContaining(r.getFromAddress());
                from.add((f == null ? "?" : f.getName()) + "@" + r.getFromAddress()
                         + ":" + r.getReferenceType());
            }
            if (!from.isEmpty())
                w.println(String.format("+0x%02X  %s  %s", off, ad, from));
        }
        w.flush(); w.close();
        println("RangeXrefs done -> " + a[0]);
    }
}
