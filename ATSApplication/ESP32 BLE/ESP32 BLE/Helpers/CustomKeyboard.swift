//
//  CustomKeyboard.swift
//  ESP32 BLE
//
//  Created by John Del Armedilla on 4/21/25.
//

import SwiftUI

struct CustomKeyboard<TextField: View, Keyboard: View>: UIViewControllerRepresentable {
    @ViewBuilder var textField: TextField
    @ViewBuilder var keyboard: Keyboard

    func makeUIViewController(context: Context) -> UIViewController {
        let controller = UIHostingController(rootView: textField)
        controller.view.backgroundColor = .clear
        
        DispatchQueue.main.async{
            if let textField = controller.view.allSubviews.first(where: { $0 is UITextField }) as? UITextField, textField.inputView == nil{
                let inputController = UIHostingController(rootView: keyboard)
                inputController.view.backgroundColor = .clear
                inputController.view.frame = .init(origin: .zero, size: inputController.view.intrinsicContentSize)
                textField.inputView = inputController.view
                textField.reloadInputViews()
            }
        }
        
        return controller
    }
    
    func updateUIViewController(_ uiViewController: UIViewControllerType, context: Context) {
        
    }
    
    func sizeThatFits(_ proposal: ProposedViewSize, uiViewController: UIViewController, context: Context) -> CGSize? {
        return uiViewController.view.intrinsicContentSize
    }
}

struct CustomKeyboardView: View{
    @Binding var text: String
    @FocusState.Binding var isActive: Bool
    
    let button_list = ["0","1","2","3","4","5","6","7","8","9","."]
    var body: some View{
        LazyVGrid(columns: Array(repeating: GridItem(spacing: 0), count: 3), spacing: 10){
            ForEach(button_list, id: \.self){ index in
                ButtonView("\(index)")
            }
            ButtonView("delete.backwards.fill", isImage: true)
            ButtonView("checkmark.circle.fill", isImage: true)
        }
        
        .padding(15)
        .background(.background.shadow(.drop(color: .black.opacity(0.08), radius: 5, x:0, y: -5)))
    }
    
    @ViewBuilder
    func ButtonView(_ value: String, isImage: Bool = false) -> some View{
        Button {
            if isImage{
                if value == "delete.backwards.fill" && !text.isEmpty{
                    // Delete last character
                    text.removeLast()
                }
                if value == "checkmark.circle.fill"{
                    // Close keyboard
                    isActive = false
                }
            } else{
                text += value
            }
        } label:{
            Group{
                if isImage {
                    Image(systemName: value)
                }
                else{
                    Text(value)
                }
            }
            .font(.title3)
            .fontWeight(.semibold)
            .frame(width: 50, height: 50)
            .background{
                if !isImage {
                    RoundedRectangle(cornerRadius: 10)
                        .fill(.background.shadow(.drop(color: .black.opacity(0.08), radius: 3, x:0, y: -5)))
                }
            }
            .foregroundStyle(Color.primary)
        }
    }
}

// Finding UITextField from the UIHosting Controller
fileprivate extension UIView{
    var allSubviews: [UIView]{
        return self.subviews.flatMap({[$0] + $0.allSubviews})
    }
}

//#Preview {
//    ContentView()
//}
