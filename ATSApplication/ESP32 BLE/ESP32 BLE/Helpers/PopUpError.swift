//
//  PopUpError.swift
//  ESP32 BLE
//
//  Created by John Del Armedilla on 4/22/25.
//

//From YouTube Video: https://www.youtube.com/watch?v=K5lj-S3grno&ab_channel=MikeMikina

import SwiftUI

struct PopUpError: View {
    
    @Binding var popupActive: Bool
    
    let title: String
    let message: String
    let buttonTitle: String
    let action: ()->()
    @State private var offset: CGFloat = 1000
    @State var graphView: Bool = false
    
    var body: some View{
        ZStack{
            Color(.black)
                .opacity(0.1)
                .onTapGesture {
                    close()
                }
            
            VStack{
                Text(title)
                    .font(.title2)
                    .bold()
                    .padding()
                
                Text(message)
                    .font(.body)
                
                Button{
                    close()
                } label:{
                    ZStack{
                        RoundedRectangle(cornerRadius: 20)
                        Text(buttonTitle)
                            .font(.system(size:16, weight:.bold))
                            .foregroundColor(.white)
                            .padding()
                    }
                    .padding()
                }
                
                
            }
            .fixedSize(horizontal: false, vertical: true)
            .padding()
            .background(.white)
            .clipShape(RoundedRectangle(cornerRadius: 20))
            .overlay{
                VStack{
                    HStack{
                        Spacer()
                        Button{
                            close()
                        } label:{
                            Image(systemName: "xmark")
                                .font(.title2)
                                .fontWeight(.medium)
                        }
                        .tint(.black)
                        
                    }
                    Spacer()
                }
                .padding()
            }
            .shadow(radius: 20)
            .padding(30)
            .offset(x:0, y: offset)
            .onAppear{
                withAnimation(.spring()){
                    offset = 0
                }
            }
        }
    }
    
    func close(){
        withAnimation(.spring()){
            offset = 1000
            popupActive = false
        }
    }
}

#Preview {
    PopUpError(popupActive: .constant(true), title: "Hello", message: "World", buttonTitle: "Yweas", action: {})
}
