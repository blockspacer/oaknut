package doxypp;

import java.util.ArrayList;
import java.util.List;

import org.w3c.dom.Node;

class CppClassSection {

	String title;
	String kind;
	String anchor;
	ArrayList<CppMethod> methods = new ArrayList<>();
	
	CppClassSection(Node node) {
		title = Xml.nt(node,  "header");
		kind = Xml.a(node, "kind");
		Node description = Xml.n(node, "description");
		if (description != null) {
			Node para = Xml.n(description, "para");
			if (para != null) {
				Node anchor = Xml.n(para, "anchor");
				if (anchor != null) {
					this.anchor = Xml.a(anchor, "id");
					int offset = this.anchor.indexOf("_1");
					if (offset > 0) {
						this.anchor = this.anchor.substring(offset+2);
					}
				}
			}
		}
		List<Node> memberDefs = Xml.ns(node, "memberdef");
		for (Node memberDef : memberDefs) {
			String memberKind = Xml.a(memberDef, "kind");
			if (memberKind.equals("public-function")
				|| memberKind.equals("function")) {
				CppMethod method = new CppMethod(memberDef);
				methods.add(method);
			}
		}
	}

	String toMarkdown() {
		String s = "";
		if (title != null && title.length()>0) {
			s += "### " + title + "\n\n";
		}
		if (methods.size() > 0) {
			for (CppMethod method : methods) {
				s += method.getSignature() + "<br>";
				if (method.brief.length() > 0) {
					s += method.brief;
				}
				s += "\n";
				if (method.detailed.length() > 0) {
					s += method.detailed;
					s += "\n";
				}
				s += "\n";
			}
			s += "\n";
		}
		return s;
	}
	String toHtml() {
		String s = "<h2>" + title + "</h2>\n\n";
		s += "<table>\n";
		for (CppMethod method : methods) {
			s += "<tr><td>";
			s += method.getSignatureHtml() + "</td>";
			s += "<td>";
			if (method.brief.length() > 0) {
				s += method.brief;
			}
			s += "</td></tr>\n";
		}
		s += "<table>\n\n";
		return s;
	}
}

